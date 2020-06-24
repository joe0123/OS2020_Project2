wget -r -np -R "index.html*" 140.112.29.204/os_data/input/
rm -rf input/
mv 140.112.29.204/os_data/input ./
rm -rf 140.112.29.204
wget -r -np -R "index.html*" 140.112.29.204/os_data/output/
rm -rf output/
mv 140.112.29.204/os_data/output ./
rm -rf 140.112.29.204
