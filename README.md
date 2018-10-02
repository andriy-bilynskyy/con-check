## Check internet connection on a specific interface
This part of software provides checking of internet connection on a specific interface.
The porpouse of using is to up/down redundant internet connection if main is lost/established.
#### Compile executable
source files location: *con-check*
necessarily libraries:
- liboping-dev
- libconfuse-dev

    make
    sudo make install
#### Using
    edit configuration file /etc/con-check/con-check.conf
    edit connection establish script. default path: /etc/con-check/online.sh
    edit connection lost script. default path: /etc/con-check/offline.sh
#### Service insatallation
    sudo cp con-check_serv /etc/init.d/
    sudo update-rc.d con-check_serv defaults
#### Usage
    sudo /etc/init.d/con-check_serv status
    sudo /etc/init.d/con-check_serv start
    sudo /etc/init.d/con-check_serv stop
    sudo /etc/init.d/con-check_serv restart