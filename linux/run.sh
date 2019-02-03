docker build -t syncmyl2p -f linux/Dockerfile .
docker run -it --rm -v %cd%:/trans syncmyl2p bash