#!/usr/bin/env python

test1 = "bob: mov a, b 		; fuuu"
test2 = "mike:mvi c, 'Q' 	; bob"
test3 = "rob mvi c, 'Q'+8;ommog zoth"
test4 = "mov coq + 5, d"
test5 = 'mov n, ","+\',\''
test6 = "mvi a, ';puu'"
test7 = "snurfel"
test8 = "sex 1,2,(3+a)"
test9 = "db '[       ]', \"*  ;'-'  *\""
test10 = "db '  \"'"

def tokenize(str, t1 = '', t2 = '', quote = None):
	if len(t1) > 0:
		yield t1
	if len(t2) > 0:
		yield t2
	if quote != None and len(str) == 0:
		raise Exception('unterminated string literal')
	for i, c in enumerate(str):
		print 'quote=', quote, ', c=', c
		if c in '\'"':
			for x in tokenize(str[i+1:], str[:i], str[i], c if quote == None else None):
				yield x
			break
		if quote != None:
			for x in tokenize(str[i+1:], str[:i], str[i], quote):
				yield x
			break
		if c in ':,;()+-/*':
			for x in tokenize(str[i+1:], str[:i], str[i]):
				yield x
			break
		elif c in ' \t':
			for x in tokenize(str[i+1:], str[:i], ''):
				yield x
			break
		elif i == len(str) - 1:
			yield str

def isInstruction(str):
	return str in {"mov", "mvi", "sex", "db"}

def joins(a1, a2):
	#if a2 == []:
	#	raise Exception('unterminated character literal')
	return a1[0:-1] + [''.join([a1[-1]] + [a2])]

def args(head, tokens, expect = None):
	#print tokens, next(tokens, []), expect
	return (lambda car, cdr: 
		args(joins(head, car), cdr, expect if car != expect else None) if expect != None 
		else args(head + [car], cdr, car) if isinstance(car, basestring) and car in "'\""
		else [head] if car == [] or car == ';'
		else [head] + args([], cdr, expect) if car == ','
		else args(head + [car], cdr, expect)) (next(tokens, []), tokens)

def instr(instr, tokens, label = None):
	return [label, instr, args([], tokens)]

def parse(tokens, label = None):
	return (lambda car, cdr:
		instr(None, cdr, label) if car == ';' or car == ''
		else parse(cdr, label) if car == ':'
		else instr(car.upper(), cdr, label) if isInstruction(car)
		else parse(cdr, label = car)) (next(tokens, ''), tokens)

print [x for x in tokenize(test1)]
print [x for x in tokenize(test2)]

print [x for x in tokenize(test10)]

print parse(tokenize(test1))
print parse(tokenize(test2))
print parse(tokenize(test3))
print parse(tokenize(test4))
print parse(tokenize(test5))
print parse(tokenize(test6))
print parse(tokenize(test7))
print parse(tokenize(test8))
print parse(tokenize(test9))
print parse(tokenize(test10))