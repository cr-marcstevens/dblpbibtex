#!/bin/bash

rm -rf tmp &>/dev/null
mkdir tmp
cd tmp

function cleanup
{
	rm *.bib *.tex *.aux &>/dev/null
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

rm test.log &>/dev/null
test_bib_download "cryptoeprint:2017:190"
if [ `grep "The first collision for full SHA-1" test.bib | wc -l` -ne 1 ]; then echo "! Failed !"; exit 1; fi
test_bib_download "DBLP:conf/crypto/StevensBKAM17"
if [ `grep "Lecture Notes in Computer Science" test.bib | wc -l` -ne 1 ]; then echo "! Failed !"; exit 1; fi
test_bib_download "dblpbibtex:enablesearch" "search-cryptoeprint:stevens+karpman"
if [ `grep "Report 2015" test.bib | wc -l` -ne 2 ]; then echo "! Failed !"; exit 1; fi
test_bib_download "DBLP:conf/icfp/HamanaF11"
if [ `grep "SIGPLAN" test.bib | wc -l` -ne 1 ]; then echo "! Failed !"; exit 1; fi
