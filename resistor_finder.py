#!/usr/bin/env python3

K = 1000
M = 1000000
RESISTORS = [
	33,
	47*K,
	470,
	3.3*K,
	150*K,
	33*K,
	510*K,
	75*K,
	3.9*K,
	220*K,
	20*K,
	5.1*K,
	49.9,
	22*K,
	51*K,
	2.2*K,
	24*K,
	1.2*K,
	5.6*K,
	8.2*K,
	300,
	680,
	39*K,
	2.4*K,
	12*K,
	220,
	51,
	27*K,
	56*K,
	1*K,
	22,
	1*M,
	200,
	15*K,
	200*K,
	1.5*K,
	120,
	150,
	510,
	470*K,
	49.9*K,
	6.8*K,
	7.5*K,
	68*K,
	120*K,
	18*K,
	4.7*K,
	2*K,
	100,
	100*K,
	330,
	10,
	75,
	10*K,
	300*K,
	1,
	47,
	330*K,
	10*M
]
R_ERROR = 0.01
VOLTAGES = [5, 9, 15, 20]
V_ERROR = 0.05
V_REF = 1.239

MIN_ERR = 100
MIN_SPREAD = 100

def vdiv(v, ra, rb):
	return v*rb / (ra+rb)

def vout(ra, rb):
	return V_REF*(ra+rb)/rb

def fb(ra, rb):
	vals = []
	vals.append(vout(ra*(1+R_ERROR), (rb)*(1+R_ERROR)))
	vals.append(vout(ra*(1+R_ERROR), (rb)*(1-R_ERROR)))
	vals.append(vout(ra*(1-R_ERROR), (rb)*(1+R_ERROR)))
	vals.append(vout(ra*(1-R_ERROR), (rb)*(1-R_ERROR)))
	return min(vals), max(vals)

def isvalid(vtarget, ra, rb):
	va, vb = fb(ra, rb)
	vmin, vmax = vtarget*(1-V_ERROR), vtarget*(1+V_ERROR)
	return va >= vmin and va <= vmax and vb >= vmin and vb <= vmax

def solve(r1, r2, r3, r4, r5):
	if not isvalid(5, r1, r2+r3+r4+r5):
		return False
	if not isvalid(9, r1, r2+r3+r4):
		return False
	if not isvalid(15, r1, r2+r3):
		return False
	if not isvalid(20, r1, r2):
		return False
	return True

def show(r1, r2, r3, r4, r5):
	global MIN_ERR
	global MIN_SPREAD
	print(f"R1 = {r1}")
	print(f"R2 = {r2}")
	print(f"R3 = {r3}")
	print(f"R4 = {r4}")
	print(f"R5 = {r5}")
	ra = r1
	rb = r2+r3+r4+r5
	rx = [r2,r3,r4,r5]
	terror = 0
	tspread = 0
	for v in VOLTAGES:
		va, vb = fb(ra, rb)
		vn = vout(ra, rb)
		rb -= rx.pop()
		error = abs(vn-v)
		spread = vb-va
		terror += error
		tspread += spread
		print(f"{v}V = {vn}V ({va}V to {vb}V)")
		print(f"    error = {error}, spread = {spread}")
	print(f"total error = {terror}, total spread = {tspread}")
	if terror < MIN_ERR:
		MIN_ERR = terror
	if tspread < MIN_SPREAD:
		MIN_SPREAD = tspread

for r1 in RESISTORS:
	for r2 in RESISTORS:
		for r3 in RESISTORS:
			for r4 in RESISTORS:
				for r5 in RESISTORS:
					if solve(r1, r2, r3, r4, r5):
						show(r1, r2, r3, r4, r5)

print(f"minimum error: {MIN_ERR}, minimum spread: {MIN_SPREAD}")
