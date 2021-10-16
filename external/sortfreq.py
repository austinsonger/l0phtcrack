import sys
import os

global freqtable

def get_frequences(words):
	global freqtable
	
	ft = {}
	
	l=1
	while l<=4:
		
		lt = {}
		
		for w in words:
			
			c=0
			while c<(len(w)-l):
				
				k = w[c:c+l]
				
				if k in lt:
					lt[k]+=1
				else:
					lt[k]=1
				
				
				c+=1
		
		ft[l] = lt
		
		l+=1
		
	freqtable=ft




def freq_cmp(x, y):
	global freqtable
	
	if len(x) < len(y):
		return -1
	if len(x) > len(y):
		return 1
	
	wl=len(x)
	
	l=len(freqtable)
	while l>0:
		
		lt = freqtable[l]
		
		c=0
		while c<(wl-l):
			
			xk = x[c:c+l]
			yk = y[c:c+l]
			
			xf = lt[xk]
			yf = lt[yk]
			
			if xf>yf:
				return -1
			if xf<yf:
				return 1
			c+=1
			
		l-=1
		
	return 0	



if __name__=="__main__":
	with open(sys.argv[1]) as f:
		words=f.readlines()
		words = [x.strip("\n") for x in words]
		
	
	get_frequences(words)
	
	l=1
	while l<=len(freqtable):
		print "# len(%d) = %d" % (l,len(freqtable[l]))
		l+=1
	
	sorted_words=sorted(words,cmp=freq_cmp)
	
	with open(sys.argv[2], "w") as f:
		for w in sorted_words:
			f.write(str(w)+"\n")
		