#!/bin/bash
rm -f paper.aux paper.log paper.bbl paper.blg paper.pdf
pdflatex paper && bibtex paper && pdflatex paper && pdflatex paper
