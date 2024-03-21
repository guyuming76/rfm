#!/bin/bash

#allow port in firewall, echo in yellow
echo -e "\e[1;33m"
read -p "Press f to share current directory temporarily in ftp, any other key to share in http: " ftp_or_http
echo "You may be asked to enter password for sudo to allow port in firewall and deny after sharing"
echo -e "\e[0m"

ip addr

echo
echo

if [[ $ftp_or_http == 'f' || $ftp_or_http == 'F' ]]; then
	sudo ufw allow 2121
	echo -e "\e[1;33m press ctrl+c to stop sharing \e[0m"
	python -m pyftpdlib -w -u user -P password
	sudo ufw deny 2121
else
	sudo ufw allow 8000
	echo -e "\e[1;33m press ctrl+c to stop sharing \e[0m"
	python -m http.server
	sudo ufw deny 8000
fi

echo -e "\e[1;33m"
read -p "press enter to quit"
echo -e "\e[0m"
