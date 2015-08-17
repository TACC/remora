#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <malloc.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <libgen.h>
#include <ev.h>
#include "ap_parse.h"
#include "x_botz.h"
#include "confuse.h"
#include "x_node.h"
#include "k_heap.h"
#include "job.h"
#include "clus.h"
#include "fs.h"
#include "lnet.h"
#include "serv.h"
#include "xltop.h"
#include "pidfile.h"
#include "trace.h"

#define XLTOP_BIND "0.0.0.0"
#define XLTOP_CLUS_INTERVAL 120.0
#define XLTOP_NR_HOSTS_HINT 4096
#define XLTOP_NR_JOBS_HINT 256
#define XLTOP_SERV_INTERVAL 300.0

#define BIND_CFG_OPTS \
  CFG_STR("bind", NULL, CFGF_NONE),           \
  CFG_STR("bind_host", NULL, CFGF_NONE),    \
  CFG_STR("bind_address", NULL, CFGF_NONE), \
  CFG_STR("bind_service", NULL, CFGF_NONE), \
  CFG_STR("bind_port", NULL, CFGF_NONE)

static int bind_cfg(cfg_t *cfg, const char *addr, const char *port)
{
  struct ap_struct ap;
  char *opt;

  opt = cfg_getstr(cfg, "bind");
  if (opt != NULL) {
    if (ap_parse(&ap, opt, addr, port) < 0)
      return -1;
    addr = ap.ap_addr;
    port = ap.ap_port;
  }

  opt = cfg_getstr(cfg, "bind_host");
  if (opt != NULL)
    addr = opt;

  opt = cfg_getstr(cfg, "bind_address");
  if (opt != NULL)
    addr = opt;

  opt = cfg_getstr(cfg, "bind_service");
  if (opt != NULL)
    port = opt;

  opt = cfg_getstr(cfg, "bind_port");
  if (opt != NULL)
    port = opt;

  if (evx_listen_add_name(&x_listen.bl_listen, addr, port, 0) < 0) {
    ERROR("cannot bind to host/address `%s', service/port `%s': %m\n",
          addr, port);
    return -1;
  }

  return 0;
}

static cfg_opt_t clus_cfg_opts[] = {
  /* AUTH_CFG_OPTS, */
  BIND_CFG_OPTS,
  CFG_STR_LIST("domains", NULL, CFGF_NONE),
  CFG_FLOAT("interval", XLTOP_CLUS_INTERVAL, CFGF_NONE),
  CFG_FLOAT("offset", 0, CFGF_NONE),
  CFG_END(),
};

static void clus_cfg(EV_P_ cfg_t *cfg, char *addr, char *port)
{
  const char *name = cfg_title(cfg);
  struct clus_node *c;

  if (bind_cfg(cfg, addr, port) < 0)
    FATAL("invalid bind option for cluster `%s'\n", name);

  c = clus_lookup(name, L_CREATE /* |L_EXCLUSIVE */);
  if (c == NULL)
    FATAL("cannot create cluster `%s': %m\n", name);

  c->c_interval = cfg_getfloat(cfg, "interval");
  /* TODO offset. */

  size_t i, nr_domains = cfg_size(cfg, "domains");
  for (i = 0; i < nr_domains; i++) {
    const char *domain = cfg_getnstr(cfg, "domains", i);
    if (clus_add_domain(c, domain) < 0)
      FATAL("cannot add domain `%s' to cluster `%s': %m\n", domain, name);
  }

  TRACE("added cluster `%s'\n", name);
}

static cfg_opt_t lnet_cfg_opts[] = {
  CFG_STR_LIST("files", NULL, CFGF_NONE),
  CFG_END(),
};

static void lnet_cfg(cfg_t *cfg, size_t hint)
{
  const char *name = cfg_title(cfg);
  struct lnet_struct *l;

  l = lnet_lookup(name, L_CREATE, hint);
  if (l == NULL)
    FATAL("cannot create lnet `%s': %m\n", name);

  size_t i, nr_files = cfg_size(cfg, "files");
  for (i = 0; i < nr_files; i++) {
    const char *path = cfg_getnstr(cfg, "files", i);
    if (lnet_read(l, path) < 0)
      FATAL("cannot read lnet file `%s': %m\n", path);
  }

  TRACE("added lnet `%s'\n", name);
}

static cfg_opt_t fs_cfg_opts[] = {
  /* AUTH_CFG_OPTS, */
  BIND_CFG_OPTS,
  CFG_STR("lnet", NULL, CFGF_NONE),
  CFG_STR_LIST("servs", NULL, CFGF_NONE),
  CFG_FLOAT("interval", XLTOP_SERV_INTERVAL, CFGF_NONE),
  CFG_END(),
};

static void fs_cfg(EV_P_ cfg_t *cfg, char *addr, char *port)
{
  const char *name = cfg_title(cfg);
  const char *lnet_name = cfg_getstr(cfg, "lnet");
  struct x_node *x;
  double interval;
  struct lnet_struct *l;
  size_t i, nr_servs;

  if (bind_cfg(cfg, addr, port) < 0)
    FATAL("fs `%s': invalid bind option\n", name); /* XXX */

  x = x_lookup(X_FS, name, x_all[1], L_CREATE);
  if (x == NULL)
    FATAL("fs `%s': cannot create filesystem: %m\n", name);

  l = lnet_lookup(lnet_name, 0, 0);
  if (l == NULL)
    FATAL("fs `%s': unknown lnet `%s': %m\n",
          name, lnet_name != NULL ? lnet_name : "-");

  interval = cfg_getfloat(cfg, "interval");
  if (interval <= 0)
    FATAL("fs `%s': invalid interval %lf\n", name, interval);

  nr_servs = cfg_size(cfg, "servs");
  if (nr_servs == 0)
    FATAL("fs `%s': no servers given\n", name);

  for (i = 0; i < nr_servs; i++) {
    const char *serv_name = cfg_getnstr(cfg, "servs", i);
    struct serv_node *s;

    s = serv_create(serv_name, x, l);
    if (s == NULL)
      FATAL("fs `%s': cannot create server `%s': %m\n", name, serv_name);

    /* TODO AUTH */

    s->s_interval = interval;
    s->s_offset = (i * interval) / nr_servs;
  }
}

static void sigterm_cb(EV_P_ ev_signal *w, int revents)
{
  ev_break(EV_A_ EVBREAK_ALL);
}

static void print_help(void)
{
  const char *p = program_invocation_short_name;

  printf("Usage: %s [OPTION]...\n"
          /* ... */
	 "Mandatory arguments to long options are mandatory for short options too.\n"
	 " -b, --bind=ADDR           listen for connections on ADDR (default %s)\n"
	 " -c, --config=DIR_OR_FILE  read configuration from DIR_OR_FILE\n"
	 " -d, --daemon              detach and run in the background\n"
	 " -h, --help                display this help and exit\n"
	 " -p, --port=PORT           listen on PORT (default %s)\n"
	 " -P, --pidfile=PATH        write PID to PATH\n"
	 " -v, --version             display version information and exit\n"
	 "\nReport %s bugs to <%s>.\n"
	 , p, XLTOP_BIND, XLTOP_PORT, p, PACKAGE_BUGREPORT);
}

static void print_version(void)
{
  printf("%s (%s) %s\n", program_invocation_short_name,
	 PACKAGE_NAME, PACKAGE_VERSION);
}

int main(int argc, char *argv[])
{
  char *b_addr = XLTOP_BIND, *b_port = XLTOP_PORT;
  const char *conf_arg = NULL;
  const char *conf_dir_path = XLTOP_CONF_DIR;
  const char *conf_file_name = NULL;
  const char *conf_file_list[] = {
    "master.conf",
    "xltop-master.conf",
  };
  FILE *conf_file = NULL;
  int pidfile_fd = -1;
  const char *pidfile_path = NULL;
  int want_daemon = 0;
  size_t i;

  struct option opts[] = {
    { "bind",     1, NULL, 'b' },
    { "config",   1, NULL, 'c' },
    { "daemon",   0, NULL, 'd' },
    { "help",     0, NULL, 'h' },
    { "port",     1, NULL, 'p' },
    { "pidfile",  1, NULL, 'P' },
    { "version",  0, NULL, 'v' },
    { NULL,       0, NULL,  0  },
  };

  int opt;
  while ((opt = getopt_long(argc, argv, "b:c:dhP:p:v", opts, 0)) > 0) {
    switch (opt) {
    case 'b':
      b_addr = optarg;
    case 'c':
      conf_arg = optarg;
      break;
    case 'd':
      want_daemon = 1;
      break;
    case 'h':
      print_help();
      exit(EXIT_SUCCESS);
    case 'P':
      pidfile_path = optarg;
      break;
    case 'p':
      b_port = optarg;
      break;
    case 'v':
      print_version();
      exit(EXIT_SUCCESS);
    case '?':
      FATAL("Try `%s --help' for more information.\n", program_invocation_short_name);
    }
  }

  /* Config! */
  cfg_opt_t main_cfg_opts[] = {
    BIND_CFG_OPTS,
    CFG_FLOAT("tick", K_TICK, CFGF_NONE),
    CFG_FLOAT("window", K_WINDOW, CFGF_NONE),
    CFG_INT("nr_hosts_hint", XLTOP_NR_HOSTS_HINT, CFGF_NONE),
    CFG_INT("nr_jobs_hint", XLTOP_NR_JOBS_HINT, CFGF_NONE),
    CFG_SEC("clus", clus_cfg_opts, CFGF_MULTI|CFGF_TITLE),
    CFG_SEC("lnet", lnet_cfg_opts, CFGF_MULTI|CFGF_TITLE),
    CFG_SEC("fs", fs_cfg_opts, CFGF_MULTI|CFGF_TITLE),
    CFG_END()
  };

  cfg_t *main_cfg = cfg_init(main_cfg_opts, 0);
  int cfg_rc;

  if (conf_arg == NULL)
    goto have_conf_dir;

  struct stat st;
  if (stat(conf_arg, &st) < 0)
    FATAL("cannot stat config dir or file `%s': %s\n",
          conf_arg, strerror(errno));

  if (S_ISDIR(st.st_mode)) {
    conf_dir_path = conf_arg;
    goto have_conf_dir;
  }

  conf_file_name = conf_arg;
  conf_file = fopen(conf_file_name, "r");
  if (conf_file == NULL)
    FATAL("cannot open config file `%s': %s\n",
          conf_file_name, strerror(errno));

  {
    char *conf_arg_tmp = strdup(conf_arg);
    char *conf_dir_tmp = dirname(conf_arg_tmp);
    conf_dir_path = strdup(conf_dir_tmp);
    free(conf_arg_tmp);
  }

have_conf_dir:
  if (chdir(conf_dir_path) < 0)
    FATAL("cannot access config dir `%s': %m\n", conf_dir_path);

  if (conf_file != NULL)
    goto have_conf_file;

  for (i = 0; i < sizeof(conf_file_list) / sizeof(conf_file_list[0]); i++) {
    conf_file_name = conf_file_list[i];
    conf_file = fopen(conf_file_name, "r");
    if (conf_file != NULL)
      goto have_conf_file;
  }

  FATAL("cannot access `%s' or `%s' in `%s': %s\n",
        conf_file_list[0], conf_file_list[1],
        conf_dir_path, strerror(errno));

have_conf_file:
  errno = 0;
  cfg_rc = cfg_parse_fp(main_cfg, conf_file);
  if (cfg_rc == CFG_FILE_ERROR) {
    if (errno == 0)
      errno = ENOENT;
    FATAL("cannot open `%s': %m\n", conf_file_name);
  } else if (cfg_rc == CFG_PARSE_ERROR) {
    FATAL("error parsing `%s'\n", conf_file_name);
  }

  k_tick = cfg_getfloat(main_cfg, "tick");
  if (k_tick <= 0)
    FATAL("%s: tick must be positive\n", conf_file_name);

  k_window = cfg_getfloat(main_cfg, "window");
  if (k_window <= 0)
    FATAL("%s: window must be positive\n", conf_file_name);

  size_t nr_host_hint = cfg_getint(main_cfg, "nr_hosts_hint");
  size_t nr_job_hint = cfg_getint(main_cfg, "nr_jobs_hint");
  size_t nr_clus = cfg_size(main_cfg, "clus");
  size_t nr_fs = cfg_size(main_cfg, "fs");
  size_t nr_serv = 0;
  size_t nr_domain = 0;

  for (i = 0; i < nr_fs; i++)
    nr_serv += cfg_size(cfg_getnsec(main_cfg, "fs", i), "servs");

  for (i = 0; i < nr_clus; i++)
    nr_domain += cfg_size(cfg_getnsec(main_cfg, "clus", i), "domains");

  x_types[X_HOST].x_nr_hint = nr_host_hint;
  x_types[X_JOB].x_nr_hint = nr_job_hint;
  x_types[X_CLUS].x_nr_hint = nr_clus;
  x_types[X_SERV].x_nr_hint = nr_serv;
  x_types[X_FS].x_nr_hint = nr_fs;

  if (x_types_init() < 0)
    FATAL("cannot initialize x_types: %m\n");

  size_t nr_listen_entries = nr_clus + nr_serv + 128; /* XXX */
  if (botz_listen_init(&x_listen, nr_listen_entries) < 0)
    FATAL("%s: cannot initialize listener\n", conf_file_name);

  x_listen.bl_conn_timeout = 600; /* XXX */

  if (bind_cfg(main_cfg, b_addr, b_port) < 0)
    FATAL("%s: invalid bind config\n", conf_file_name);

  for (i = 0; i < NR_X_TYPES; i++)
    if (x_dir_init(i, NULL) < 0)
      FATAL("cannot initialize type resources: %m\n");

  if (serv_type_init() < 0)
    FATAL("cannot initialize serv type: %m\n");

  if (clus_type_init(nr_domain) < 0)
    FATAL("cannot initialize default cluster: %m\n");

  if (fs_type_init() < 0)
    FATAL("cannot initialize fs type: %m\n");

  for (i = 0; i < nr_clus; i++)
    clus_cfg(EV_DEFAULT_
             cfg_getnsec(main_cfg, "clus", i),
             b_addr, b_port);

  size_t nr_lnet = cfg_size(main_cfg, "lnet");
  for (i = 0; i < nr_lnet; i++)
    lnet_cfg(cfg_getnsec(main_cfg, "lnet", i), nr_host_hint);

  for (i = 0; i < nr_fs; i++)
    fs_cfg(EV_DEFAULT_
           cfg_getnsec(main_cfg, "fs", i),
           b_addr, b_port);

  cfg_free(main_cfg);
  fclose(conf_file);

  extern const struct botz_entry_ops top_entry_ops; /* MOVEME */
  if (botz_add(&x_listen, "top", &top_entry_ops, NULL) < 0)
    FATAL("cannot add listen entry `%s': %m\n", "top");

  extern const struct botz_entry_ops domains_entry_ops; /* MOVEME */
  if (botz_add(&x_listen, "_domains", &domains_entry_ops, NULL) < 0)
    FATAL("cannot add listen entry `%s': %m\n", "_domains");

  signal(SIGPIPE, SIG_IGN);

  evx_listen_start(EV_DEFAULT_ &x_listen.bl_listen);

  if (want_daemon)
    daemon(0, 0);
  else
    chdir("/");

  if (pidfile_path != NULL) {
    pidfile_fd = pidfile_create(pidfile_path);
    if (pidfile_fd < 0)
      FATAL("exiting\n");
  }

  static struct ev_signal sigterm_w;
  ev_signal_init(&sigterm_w, &sigterm_cb, SIGTERM);
  ev_signal_start(EV_DEFAULT_ &sigterm_w);

  ev_run(EV_DEFAULT_ 0);

  if (pidfile_path != NULL)
    unlink(pidfile_path);

  FATAL("exiting\n");
}
