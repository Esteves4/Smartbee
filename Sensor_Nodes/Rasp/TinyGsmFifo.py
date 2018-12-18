class TinyGsmFifo:
	def __init__(self, N):
		self.w = 0
		self.r = 0
		self.N = N
		self.b = [None]*self.N

	def clear(self):
		self.w = 0
		self.r = 0

	def writeable(self):
		return self.free() > 0

	def free(self):
		s = self.r - self.w

		if s <= 0:
			s += self.N

		return s - 1

	def put(self, p, n = 1, t = False):
		if n == 1:
			i = self.w
			j = i

			i = self.inc(i)
			if(i == self.r): # not writeable()
				return False

			self.b[j] = ord(p)
			self.w = i
			return True

		c = n
		contP = 0
		while(c):
			f = self.free()
			while(f == 0): #Wait for space
				if (not t):
					return n - c #No mode space and not blocking
				f = self.free()
				# nothing / just wait

			#Check free sparece
			if (c < f):
				f = c

			w = self.w
			m = self.N - w

			# Check wrap
			if ( f > m):
				f = m
			#self.b[w:w+f] = p[contP:contP+f]
			for i in range(w,f):
				self.b[i] = ord(p[contP])
				contP += 1
			self.w = self.inc(w,f)
			c -= f
			#contP += f

		return n - c, p

	def readable(self):
		return self.r != self.w

	def size(self):
		s = self.w - self.r
		if s < 0:
			s += self.N

		return s
	def get(self, p, n = 1, t = False):
		if n == 1:
			r = self.r
			if r == self.w: # not readable()
				return False, None
			p = self.b[r]
			self.r = self.inc(r)
			return True, p

		c = n
		contP = 0
		while(c):
			f = None

			while(1): #Wait for data
				f = self.size()
				if(f): break            #Free space
				if(not t): return n - c, None # No space and not blocking
				# nothing / just wait

			# Check available data

			if (c < f): f = c
			r = self.r
			m = self.N - r

			#Check wrap
			if(f > m): f = m
			p[contP:contP+f] = self.b[r:r+f]
			self.r = self.inc(r, f)
			c -= f
			contP += f

		return n - c, p

	def inc(self, i, n = 1):
		return (i + n) % self.N
			



