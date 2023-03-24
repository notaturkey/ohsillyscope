# ohsillyscope
making this project open source for the peeps \
Tag me if you use it! \
instagram: @thomas_is_lame

Sorry in advance if this readme is too vague, i made this project months ago and forgot all the initial setup i had to do.\
if you wanna make this repo better please submit a pr and ill gladly let people collaborate!

## how to build and run the ohsillyscope
This repo is based off the https://github.com/hzeller/rpi-rgb-led-matrix project, so youll have to already have the matrix and raspberry pi setup per there instructions, the rest is pretty easy to get going.

clone this repo into your pi\
clone the submodule 
```
git submodule init 
git submodule update
```
make the matrix submodule (this might take a bit of effort, i forgot most of the libraries i had to install on my pi)
```
cd ./matrix
make
```
then make the osc project (again here you might need to install some libraries, i completly forgot what i installed to get it working. If someone does this and wants to make this readme better please submit a pr)
```
cd ..
make
```
finally were ready to run the ohsillyscope\
the code uses alsa to interpret an audio signal, so you have to pass which audio card it is reading from.\
to find your audio interface, run
```
aplay -l
```
then use the card number that corresponds to the audio you want to capture. In my case my interface shows up as card 1 so the startup command is:
```
./osc "hw:1"
```

have fun :)

