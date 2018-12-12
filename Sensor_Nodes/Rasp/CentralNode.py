class TinyGsmFifo:
	def __init__(self, N):
		self.w = 0
		self.r = 0
		self.N = N
		self.b = [None]*N

	def clear(self):
		self.w = 0
		self.r = 0

	def writeable(self):
		return self.free() > 0

	def free(self):
		s = self.r - self.w

		if s <= 0:
			s += n

		return s - 1

	def put(self, c):
		i = self.w
		j = self.i

		i = self.inc(i)

		if(i == self.r): # not writeable()
			return False

		self.b[j] = c
		self.w = i
		return True

	def put(self, p, n, t = False):
		c = n

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
			self.b[w:w+f] = p[0:f]
			self.w = self.inc(w,f)
			c -= f
			p += f

		return n - c

	def readable(self):
		return self.r != self.w

	def size(self):
		s = self.w - self.r
		if s > 0:
			s += self.N

		return s

	def get(self, p):
		r = self.r
		if r == w: # not readable()
			return False
		p.append(self.b[r])

		self.r = self.inc(r)

		return True

	def get(self, p, n, t = False):
		c = n

		while(c):
			f = None

			while(1): #Wait for data
				f = self.size()
				if(f): break            #Free space
				if(not t): return n - c # Bi soace abd bit blocking
				# nothing / just wait

			# Check available data

			if (c < f): f = c
			r = self.r
			m = self.N - r

			#Check wrap
			if(f > m): f = m
			p[0:f] = self.b[r:r+f]
			self.r = self.inc(r, f)
			c -= f
			p += f

		return n - c

	def inc(self, i, n = 1):
		return (i + n) % self.N
			



