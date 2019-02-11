#!/usr/bin/python

import time 
import datetime 
import os

def zipFiles(path_source, path_target, timestamp, ident):
	# Verifica se existe arquivo txt com data anterior, se existir, comprime e exclui o arquivo txt
	timestamp = timestamp - datetime.timedelta(days=1) 

	filename_tar = timestamp.strftime("%d_%m_%y") + "_" + str(ident)
	filename_txt = timestamp.strftime("%d_%m_%y")

	if os.path.exists(path_source + filename_txt + ".txt"):
		os.system("sudo tar -C " + path_source + " -zcvf " + path_target + filename_tar + ".tar.gz ./" + filename_txt + ".txt --remove-files")
	else:
		print("Arquivo " + path_source + filename_txt + ".txt nao foi encontrado")

timestamp = datetime.datetime.now()

path_s = os.path.dirname(os.path.realpath(__file__))
path_t = "/usr/share/nginx/html/dados/"

zipFiles(path_s + "/data_collect/", path_t, timestamp, "data")
zipFiles(path_s + "/audio_collect/", path_t, timestamp, "audio")
