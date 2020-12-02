# spectre_project
## Environment Setup for Android Smartphones
Install Termux App from Google Play or any app stores.

Install clang for C compiler, and install git for getting codes from github:
```
pkg install clang
pkg install git
```

Then clone the codes from the repo and cd to that directory:
```
git clone https://github.com/prakashmurali/spectre_project.git
cd spectre_project
```

Comepile the files using gcc:
```
gcc -o ct cache_time.c
gcc -o fr flush_reload.c
gcc -o sp spectre.c
gcc -o si spectre_improved.c
```
Execute the Program you need, use 10 as an example cache hit threshold:
```
./ct
./fr 10
./sp 10
./si 10
```

## Functionalities of Each Program 
cache_time.c studies the cache features of ARM processors on smartphones, we can get the cache hit threshold from this program.

flush_reload.c does the flush and reload attack, it needs an input to set the cache hit threshold.

spectre.c finishes the Spectre V1 attack, it also needs an input to set the cache hit threshold.

spectre_improved.c has a improved version of the attack which can reach the 99.9% success rate, it also needs an input to set the cache hit threshold.
