#include "consts.h"
.include "fq.inc"

.text
reduce128_avx:
#load
vmovdqa		(%rdi),%ymm2
vmovdqa		32(%rdi),%ymm3
vmovdqa		64(%rdi),%ymm4
vmovdqa		96(%rdi),%ymm5
vmovdqa		128(%rdi),%ymm6
vmovdqa		160(%rdi),%ymm7
vmovdqa		192(%rdi),%ymm8
vmovdqa		224(%rdi),%ymm9

red16		2,x=10
red16		3,x=11
red16		4,x=12
red16		5,x=13
red16		6,x=14
red16		7,x=15
red16		8,x=10
red16		9,x=11

#store
vmovdqa		%ymm2,(%rdi)
vmovdqa		%ymm3,32(%rdi)
vmovdqa		%ymm4,64(%rdi)
vmovdqa		%ymm5,96(%rdi)
vmovdqa		%ymm6,128(%rdi)
vmovdqa		%ymm7,160(%rdi)
vmovdqa		%ymm8,192(%rdi)
vmovdqa		%ymm9,224(%rdi)

ret

.global cdecl(reduce_avx)
cdecl(reduce_avx):
#consts
vmovdqa		_16XQ*2(%rsi),%ymm0
vmovdqa		_16XV*2(%rsi),%ymm1
call		reduce128_avx
add		$256,%rdi
call		reduce128_avx
ret

csubq128_avx:
#load
vmovdqa		(%rdi),%ymm1
vmovdqa		32(%rdi),%ymm2
vmovdqa		64(%rdi),%ymm3
vmovdqa		96(%rdi),%ymm4
vmovdqa		128(%rdi),%ymm5
vmovdqa		160(%rdi),%ymm6
vmovdqa		192(%rdi),%ymm7
vmovdqa		224(%rdi),%ymm8

csubq		1,9
csubq		2,10
csubq		3,11
csubq		4,12
csubq		5,13
csubq		6,14
csubq		7,15
csubq		8,9

#store
vmovdqa		%ymm1,(%rdi)
vmovdqa		%ymm2,32(%rdi)
vmovdqa		%ymm3,64(%rdi)
vmovdqa		%ymm4,96(%rdi)
vmovdqa		%ymm5,128(%rdi)
vmovdqa		%ymm6,160(%rdi)
vmovdqa		%ymm7,192(%rdi)
vmovdqa		%ymm8,224(%rdi)

ret

.global cdecl(csubq_avx)
cdecl(csubq_avx):
#consts
vmovdqa		_16XQ*2(%rsi),%ymm0
call		csubq128_avx
add		$256,%rdi
call		csubq128_avx
ret

tomont128_avx:
#load
vmovdqa		(%rdi),%ymm3
vmovdqa		32(%rdi),%ymm4
vmovdqa		64(%rdi),%ymm5
vmovdqa		96(%rdi),%ymm6
vmovdqa		128(%rdi),%ymm7
vmovdqa		160(%rdi),%ymm8
vmovdqa		192(%rdi),%ymm9
vmovdqa		224(%rdi),%ymm10

fqmulprecomp	1,2,3,11
fqmulprecomp	1,2,4,12
fqmulprecomp	1,2,5,13
fqmulprecomp	1,2,6,14
fqmulprecomp	1,2,7,15
fqmulprecomp	1,2,8,11
fqmulprecomp	1,2,9,12
fqmulprecomp	1,2,10,13

#store
vmovdqa		%ymm3,(%rdi)
vmovdqa		%ymm4,32(%rdi)
vmovdqa		%ymm5,64(%rdi)
vmovdqa		%ymm6,96(%rdi)
vmovdqa		%ymm7,128(%rdi)
vmovdqa		%ymm8,160(%rdi)
vmovdqa		%ymm9,192(%rdi)
vmovdqa		%ymm10,224(%rdi)

ret

.global cdecl(tomont_avx)
cdecl(tomont_avx):
#consts
vmovdqa		_16XQ*2(%rsi),%ymm0
vmovdqa		_16XMONTSQLO*2(%rsi),%ymm1
vmovdqa		_16XMONTSQHI*2(%rsi),%ymm2
call		tomont128_avx
add		$256,%rdi
call		tomont128_avx
ret
