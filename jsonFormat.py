import json
import numpy as np

def writeJson(idnum,name,dim,start): 
	sensor={"sensor_id":idnum}
	sensor["sensor_type"]=name
	sensor["dimension"]=dim
	sensor["sample_rate"]=Hz

	data=[]
	for i in range(len(alldata)-1):
		temp=[];
		temp.append(alldata[i][0])
		for j in range(dim):
			temp.append(alldata[i][start+j])
		data.append(temp)

	sensor["data"]=data;
	sensors.append(sensor);

Hz="5Hz"

file=open('0.txt')
alldata=[];
sensors=[];

while 1:
	s=file.readline()
	if s=='' :
		break
	arr=s.split(' ')
	arr[-1]=arr[-1].replace('\n','')
	alldata.append(arr)
file.close()
root={"trowel_id":1};
root["StartTime"]=alldata[0][0];

writeJson(0,"Temperature",1,1);
writeJson(1,"Accelerometer",3,2);
writeJson(2,"Magnemtometer",3,5);
writeJson(3,"Gyroscope",3,8);
writeJson(4,"YawRollPitch",3,11)
writeJson(5,"LinearAcceleration",3,18)
writeJson(6,"Gravity",3,21)
writeJson(7,"XYZPosition",3,23)

root["sensors"]=sensors

file=open('datalogJson.txt', 'w')
file.write(json.dumps(root, sort_keys=False, indent=2))
file.close()


