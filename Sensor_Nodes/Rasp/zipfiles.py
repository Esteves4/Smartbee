import time 
import datetime 
import os

def zipFiles(path, timestamp):
	# Verifica se existe arquivo txt com data anterior, se existir, comprime e exclui o arquivo txt
	timestamp = timestamp - datetime.timedelta(days=1) 

	filename = timestamp.strftime("%d_%m_%y")

	if os.path.exists(path + filename + ".txt"):
		os.system("sudo tar -C " + path + " -zcvf " + path + filename + ".tar.gz ./" + filename + ".txt --remove-files")
	
timestamp = datetime.datetime.now()

zipFiles("data_collect/", timestamp)
zipFiles("audio_collect/", timestamp)