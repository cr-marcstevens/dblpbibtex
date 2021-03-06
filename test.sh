#!/bin/bash

rm -rf tmp
mkdir tmp
cd tmp

function cleanup
{
	rm *.bib *.tex *.aux
}
function make_tex_doc
{
echo "\documentclass{article}" > test.tex
echo "\usepackage{url}" >> test.tex
echo "\begin{document}" >> test.tex
while [ "$1" != "" ]; do
	echo "\cite{$1}" >> test.tex
	shift 1
done
echo "\bibliographystyle{plain}" >> test.tex
echo "\bibliography{test}" >> test.tex
echo "\end{document}" >> test.tex
}

function test_bib_download
{
cleanup
make_tex_doc $*
touch test.bib

pdflatex test &> pdflatex1.log
../dblpbibtex test &>> dblpbibtex.log
#echo "=== test.tex ==="
#cat test.tex
#echo "================"
#echo "=== test.bib ==="
#cat test.bib
#echo "================"

pdflatex test &> pdflatex2.log
../dblpbibtex test &>> dblpbibtex.log
echo "=== test.tex ==="
cat test.tex
echo "================"
echo "=== test.bib ==="
cat test.bib
echo "================"

}

rm test.log
test_bib_download "cryptoeprint:2017:190"
test_bib_download "DBLP:conf/crypto/StevensBKAM17"
test_bib_download "dblpbibtex:enablesearch" "search-cryptoeprint:stevens+karpman"
test_bib_download "DBLP:conf/icfp/HamanaF11"
