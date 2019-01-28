#!/usr/bin/python

import time 
import datetime 
import os

def zipFiles(path, timestamp, ident):
	# Verifica se existe arquivo txt com data anterior, se existir, comprime e exclui o arquivo txt
	timestamp = timestamp - datetime.timedelta(days=1) 

	filename_tar = timestamp.strftime("%d_%m_%y") + "_" + str(ident)
	filename_txt = timestamp.strftime("%d_%m_%y")

	if os.path.exists(path + filename_txt + ".txt"):
		os.system("sudo tar -C " + path + " -zcvf " + "/srv/ftp/" + filename_tar + ".tar.gz ./" + filename_txt + ".txt --remove-files")
	
timestamp = datetime.datetime.now()

zipFiles("/home/pi/Documents/Smartbee/Sensor_Nodes/Rasp/data_collect/", timestamp, "data")
zipFiles("/home/pi/Documents/Smartbee/Sensor_Nodes/Rasp/audio_collect/", timestamp, "audio")
