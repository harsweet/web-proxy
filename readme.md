# Steps for successfully running the HTTP Proxy

##  Configuring the Browser

- Open the Network Settings of your browser
- Now turn Manual Proxy Configuration ON
- Change the HTTP Proxy configuration
- Set IP address to 127.0.0.1 (your localhost address) and port number to 50000
- Set HTTPS enforcement (if any) off
- Save the settings

##  Running the Proxy

- Open Terminal at the folder containing the http_proxy.cpp file
- Now run the following the commands
```sh
g++ http_proxy.cpp -o p.o
./p.o
```
- Enter the word to be replaced and the replacement
- Now you can browse any HTTP website in your browser and the proxy should work in doing the word replacement


# Some HTTP Websites that you can try testing the proxy on

- http://info.cern.ch/
- http://milk.com/
- http://itcorp.com/
  

