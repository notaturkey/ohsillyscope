# ohsillyscope
making this project open source for the peeps \
Tag me if you use it! \
instagram: @thomas_is_lame

Sorry in advance if this readme is too vague, i made this project months ago and forgot all the initial setup i had to do.\
if you wanna make this repo better please submit a pr and ill gladly let people collaborate!

## how to build and run the ohsillyscope
This repo is based off the https://github.com/hzeller/rpi-rgb-led-matrix project, so youll have to already have the matrix and raspberry pi setup per their instructions, the rest is pretty easy to get going.

clone this repo into your pi\
clone the submodule 
```
git submodule init 
git submodule update
```
make the matrix submodule
```
cd ./matrix
make
```
install some needed libraries for this project to compile
```
sudo apt-get install libasound2-dev
sudo apt-get install libncurses-dev
```
grant access to the interface youre using
```
sudo adduser <username> audio
```
then make the osc project
```
cd ..
make
```
finally were ready to run the ohsillyscope\
the code uses alsa/pulseaudio to interpret an audio signal, so you have to pass which audio card it is reading from.\
to find your audio interface, run
```
aplay -l
```
then use the card number that corresponds to the audio you want to capture. In my case my interface shows up as card 1 so the startup command is:
```
./osc "hw:1"
```

have fun :)

