import sys

# Usage: IPGenerator.py <NUMBER> (a|b)
# NOTE: This program will calculate and print 10.x.x.x/24 IP address for use in Virtual Ethernet (veth)
# NOTE: 10.x.x.0 and 10.x.x.255 will not be used as they are network and broadcast IP addresses

# print(sys.argv)
if len(sys.argv) == 3:
	try:
		num = int(sys.argv[1])
		# print(num)
		a,b,c,d=10,0,num,1
		b = c // 256
		c %= 256
		if b > 255:
			print("ERROR: IP went Outside limit: input={}, a={}, b={}, c={}, d={}".format(sys.argv[1], a,b,c,d))
			sys.exit(1)
		if sys.argv[2] in ('0', 'a'):
			val_to_add_to_ip = 0
		else:
			val_to_add_to_ip = 1
		print("{}.{}.{}.{}/24".format(a, b, c, d+val_to_add_to_ip))
	except Exception as e:
		print("ERROR: {}: {}".format(type(e), str(e)))
else:
	print("ERROR: invalid syntax\nUsage: IPGenerator.py <NUMBER> (a|b)")