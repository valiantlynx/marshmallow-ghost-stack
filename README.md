# ghost stack inspired "how to roast a marchmallow game.
for the beta uia Gamejam

g++ -o marshmallow_game main.cpp -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

./marshmallow_game

#installing raylib first
tar -xvzf raylib-5.0_linux_amd64.tar.gz
cd raylib-5.0_linux_amd64/

sudo cp include/raylib.h /usr/local/include/
sudo cp include/raymath.h /usr/local/include/
sudo cp include/rlgl.h /usr/local/include/

sudo cp libraylib.a /usr/local/lib/
sudo cp libraylib.so /usr/local/lib/

sudo apt-get install libsqlite3-dev make