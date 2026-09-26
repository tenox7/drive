#!/bin/sh

FILES=$(echo *.wiki | sed 's/.wiki//g')

for i in $FILES
do

IN=$i.wiki
OUT=../HTML/$i.htm

if [ $IN -nt $OUT ]
then

echo $i

TITLE=$(head -2 $IN | tail -1)

echo >$OUT '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0//EN">'
echo >>$OUT '<html>'
echo >>$OUT '<head>'
echo >>$OUT '<meta http-equiv="Content-Type" content="text/html;charset=utf-8">'
echo >>$OUT '<title>'$TITLE'</title>'
echo >>$OUT '<link rel="stylesheet" type="text/css" href="style.css">'
echo >>$OUT '</head>'
echo >>$OUT '<body>'

nme --body --easylink '#$' <$IN | sed -f sed.txt>>$OUT

echo >>$OUT '</body>'
echo >>$OUT '</html>'

fi

done
