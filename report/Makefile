filename=main

pdf:
	pdflatex ${filename}
	pdflatex ${filename}
	make clean
	mv ./main.pdf ../out/report.pdf

read:
	evince ../out/report.pdf &


clean:
	rm -f ${filename}.out ${filename}.log ${filename}.bbl ${filename}.blg ${filename}.au ${filename}.log  ${filename}.ps ${filename}.aux ${filename}.out ${filename}.dvi ${filename}.bbl ${filename}.blg ${filename}.toc  ${filename}.nav ${filename}.vrb ${filename}.snm
