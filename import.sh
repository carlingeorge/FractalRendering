palex -f $1 -o ./palette
cd palette
cd $2
mv * ../
cd ..
rm -r $2