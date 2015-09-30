# Makefile for the remora_user_guide document in LaTex format

all: remora_user_guide.pdf

.PHONY: all quick debug clean cleanall

remora_user_guide.pdf: remora_user_guide.tex  Makefile
	-pdflatex -interaction=batchmode -file-line-error remora_user_guide.tex
	-pdflatex -interaction=batchmode -file-line-error remora_user_guide.tex
	-pdflatex -interaction=batchmode -file-line-error remora_user_guide.tex

quick:
	pdflatex -interaction=batchmode  -file-line-error remora_user_guide.tex

debug:
	pdflatex                         -file-line-error remora_user_guide.tex

clean:
	rm -f remora_user_guide.toc remora_user_guide.out remora_user_guide.log remora_user_guide.aux

cleanall:
	rm -f remora_user_guide.pdf remora_user_guide.toc remora_user_guide.out remora_user_guide.log remora_user_guide.aux
