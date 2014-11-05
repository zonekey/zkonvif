import sys
import json,io
import time
sys.path.append('../')
from common.utils import zkutils

# heart beat time interval, unit is second
HB_TIME = 10

# get ip and port of host about register and heart beat 
def getRgHbSv(f):
	ret = json.load(io.open('config.json', 'r', encoding='utf-8'))
	return ret['regHbService']

# get ip and port of local host
def getLocalHost():
	u = zkutils()
	localIp = u.myip_real()
	localMac = u.mymac()
	localHost = {}
	localHost['localIp'] = localIp
	localHost['localMac'] = localMac
	return localHost

import urllib2

# get send string about register and heart beat 
def getUrl(client_params, fun_str):
	v =	client_params
	ret = 'http://' + v['sip'] + ':' + v['sport'] + '/' + fun_str + '=' +v['localIp'] + '_' + v['localMac'] + '_' + v['type'] + '_' + v['id'] + '_' + v['url']
	return ret

def register(client_params):
	regUrl = getUrl(client_params, 'deviceService/registering?serviceinfo')
	s = urllib2.urlopen(regUrl)
	print '============>register'
	print regUrl
	str = s.read(1000).decode('utf-8')
	ret = json.loads(str)
	v = client_params['localIp'] + '_' + client_params['localMac'] + '_' + client_params['type'] + '_' + client_params['id']

	if v in ret['info']:
		return True
	else:
		return False

def heartBeat(service_params):
	heartBeatUrl = getUrl(service_params, 'deviceService/heartbeat?serviceinfo')
	s = urllib2.urlopen(heartBeatUrl)
	print '===========>heartbeat'
	print heartBeatUrl

import threading


class RegClass(threading.Thread):
	def __init__(self, pcs_paras, services, mtx_reg, mtx_hb):
		threading.Thread.__init__(self)
		self.pcs_paras_ = pcs_paras
		self.services_ = services
		self.mtx_reg_ = mtx_reg
		self.mtx_hb_ = mtx_hb
		self.service_ = {}
		self.service_.update(getRgHbSv('config.json'))
		self.service_.update(getLocalHost())
	def run(self):
		while True:
			pcs_paras = []
			self.mtx_reg_.acquire()
			for e in self.pcs_paras_:
				pcs_paras.append(e)
			self.mtx_reg_.release()
			for e in pcs_paras:
				service = {}
				service.update(self.service_)
				service.update(e)
				f = urllib2.urlopen(service['url'] + r'/internal?command=services')
				jv = f.read(1000)
				dv = json.loads(jv, 'utf-8')
				if dv['complete']: 
					for e1 in dv['ids']:
						isReg = False
						service['id'] = e1
						while not isReg: # if can't register service, loop until register it
							if register(service): 
								isReg = True
						heartBeat(service)
						sv = {}
						sv.update(service)
						self.mtx_hb_.acquire()
						self.services_.append(sv)
						self.mtx_hb_.release()
					self.mtx_reg_.acquire()
					self.pcs_paras_.remove(e) # only dv['complete'] is Ture, remove e(process)
					self.mtx_reg_.release()			

class HbClass(threading.Thread):
	def __init__(self, services, mtx_hb):
		threading.Thread.__init__(self)
		self.services_ = services
		self.mtx_hb_ = mtx_hb
		
	def run(self):
		while True:
			services = []	
			self.mtx_hb_.acquire()		
			for e in self.services_:
				services.append(e)
			self.mtx_hb_.release()
			for e in services:
				heartBeat(e)
			time.sleep(HB_TIME)