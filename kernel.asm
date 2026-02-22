
kernel.bin:     file format binary


Disassembly of section .data:

00000000 <.data>:
       0:	02 b0 ad 1b 03 00    	add    0x31bad(%eax),%dh
       6:	00 00                	add    %al,(%eax)
       8:	fb                   	sti
       9:	4f                   	dec    %edi
       a:	52                   	push   %edx
       b:	e4 00                	in     $0x0,%al
       d:	00 00                	add    %al,(%eax)
       f:	00 55 89             	add    %dl,-0x77(%ebp)
      12:	e5 56                	in     $0x56,%eax
      14:	53                   	push   %ebx
      15:	66 b8 10 00          	mov    $0x10,%ax
      19:	8e d8                	mov    %eax,%ds
      1b:	8e c0                	mov    %eax,%es
      1d:	8e e0                	mov    %eax,%fs
      1f:	8e e8                	mov    %eax,%gs
      21:	8e d0                	mov    %eax,%ss
      23:	e8 f8 04 00 00       	call   0x520
      28:	31 c0                	xor    %eax,%eax
      2a:	c7 05 20 68 00 00 00 	movl   $0x0,0x6820
      31:	00 00 00 
      34:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
      38:	c6 80 40 68 00 00 00 	movb   $0x0,0x6840(%eax)
      3f:	83 c0 02             	add    $0x2,%eax
      42:	c6 80 3f 68 00 00 00 	movb   $0x0,0x683f(%eax)
      49:	83 f8 20             	cmp    $0x20,%eax
      4c:	75 ea                	jne    0x38
      4e:	c7 05 34 59 00 00 0f 	movl   $0xf,0x5934
      55:	00 00 00 
      58:	31 f6                	xor    %esi,%esi
      5a:	8d 1c f6             	lea    (%esi,%esi,8),%ebx
      5d:	31 d2                	xor    %edx,%edx
      5f:	8d 04 dd c0 6a 00 00 	lea    0x6ac0(,%ebx,8),%eax
      66:	c7 40 08 00 00 00 00 	movl   $0x0,0x8(%eax)
      6d:	c6 04 dd c0 6a 00 00 	movb   $0x0,0x6ac0(,%ebx,8)
      74:	00 
      75:	01 db                	add    %ebx,%ebx
      77:	c6 40 01 00          	movb   $0x0,0x1(%eax)
      7b:	c6 40 02 00          	movb   $0x0,0x2(%eax)
      7f:	c6 40 03 00          	movb   $0x0,0x3(%eax)
      83:	c6 40 04 00          	movb   $0x0,0x4(%eax)
      87:	c6 40 05 00          	movb   $0x0,0x5(%eax)
      8b:	c6 40 06 00          	movb   $0x0,0x6(%eax)
      8f:	c6 40 07 00          	movb   $0x0,0x7(%eax)
      93:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
      97:	90                   	nop
      98:	8d 04 13             	lea    (%ebx,%edx,1),%eax
      9b:	83 c2 01             	add    $0x1,%edx
      9e:	c7 04 85 cc 6a 00 00 	movl   $0x0,0x6acc(,%eax,4)
      a5:	00 00 00 00 
      a9:	8d 48 08             	lea    0x8(%eax),%ecx
      ac:	83 c0 0c             	add    $0xc,%eax
      af:	c7 04 8d c0 6a 00 00 	movl   $0x0,0x6ac0(,%ecx,4)
      b6:	00 00 00 00 
      ba:	c7 04 85 c4 6a 00 00 	movl   $0x0,0x6ac4(,%eax,4)
      c1:	00 00 00 00 
      c5:	83 fa 05             	cmp    $0x5,%edx
      c8:	75 ce                	jne    0x98
      ca:	83 c6 01             	add    $0x1,%esi
      cd:	83 fe 04             	cmp    $0x4,%esi
      d0:	75 88                	jne    0x5a
      d2:	c6 05 18 6d 00 00 00 	movb   $0x0,0x6d18
      d9:	b9 a0 6c 00 00       	mov    $0x6ca0,%ecx
      de:	31 d2                	xor    %edx,%edx
      e0:	c7 05 1c 6d 00 00 00 	movl   $0x0,0x6d1c
      e7:	00 00 00 
      ea:	89 c8                	mov    %ecx,%eax
      ec:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
      f0:	89 50 08             	mov    %edx,0x8(%eax)
      f3:	83 c2 01             	add    $0x1,%edx
      f6:	83 c0 18             	add    $0x18,%eax
      f9:	c7 40 e8 00 00 00 00 	movl   $0x0,-0x18(%eax)
     100:	c7 40 ec 00 00 00 00 	movl   $0x0,-0x14(%eax)
     107:	c7 40 f4 64 00 00 00 	movl   $0x64,-0xc(%eax)
     10e:	c7 40 f8 00 00 00 00 	movl   $0x0,-0x8(%eax)
     115:	83 fa 05             	cmp    $0x5,%edx
     118:	75 d6                	jne    0xf0
     11a:	c6 05 98 6d 00 00 00 	movb   $0x0,0x6d98
     121:	b8 a0 6c 00 00       	mov    $0x6ca0,%eax
     126:	c7 05 9c 6d 00 00 00 	movl   $0x0,0x6d9c
     12d:	00 00 00 
     130:	89 90 88 00 00 00    	mov    %edx,0x88(%eax)
     136:	83 c2 01             	add    $0x1,%edx
     139:	83 c0 18             	add    $0x18,%eax
     13c:	c7 40 68 00 00 00 00 	movl   $0x0,0x68(%eax)
     143:	c7 40 6c 00 00 00 00 	movl   $0x0,0x6c(%eax)
     14a:	c7 40 74 64 00 00 00 	movl   $0x64,0x74(%eax)
     151:	c7 40 78 00 00 00 00 	movl   $0x0,0x78(%eax)
     158:	83 fa 0a             	cmp    $0xa,%edx
     15b:	75 d3                	jne    0x130
     15d:	c7 05 e0 6b 00 00 00 	movl   $0x0,0x6be0
     164:	00 00 00 
     167:	ba a0 6c 00 00       	mov    $0x6ca0,%edx
     16c:	31 c0                	xor    %eax,%eax
     16e:	c7 05 f4 6b 00 00 ed 	movl   $0x55ed,0x6bf4
     175:	55 00 00 
     178:	c7 05 ec 6b 00 00 1e 	movl   $0x1e,0x6bec
     17f:	00 00 00 
     182:	c7 05 e8 6b 00 00 00 	movl   $0x0,0x6be8
     189:	00 00 00 
     18c:	c7 05 40 6c 00 00 01 	movl   $0x1,0x6c40
     193:	00 00 00 
     196:	c7 05 54 6c 00 00 f2 	movl   $0x55f2,0x6c54
     19d:	55 00 00 
     1a0:	c7 05 4c 6c 00 00 3c 	movl   $0x3c,0x6c4c
     1a7:	00 00 00 
     1aa:	c7 05 48 6c 00 00 01 	movl   $0x1,0x6c48
     1b1:	00 00 00 
     1b4:	c7 05 e4 6b 00 00 01 	movl   $0x1,0x6be4
     1bb:	00 00 00 
     1be:	c7 05 f0 6b 00 00 e8 	movl   $0x3e8,0x6bf0
     1c5:	03 00 00 
     1c8:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
     1cf:	90                   	nop
     1d0:	c7 04 85 f8 6b 00 00 	movl   $0x0,0x6bf8(,%eax,4)
     1d7:	00 00 00 00 
     1db:	83 c2 18             	add    $0x18,%edx
     1de:	c7 04 85 0c 6c 00 00 	movl   $0x64,0x6c0c(,%eax,4)
     1e5:	64 00 00 00 
     1e9:	c7 04 85 20 6c 00 00 	movl   $0x3e8,0x6c20(,%eax,4)
     1f0:	e8 03 00 00 
     1f4:	83 c0 01             	add    $0x1,%eax
     1f7:	c7 42 fc e8 03 00 00 	movl   $0x3e8,-0x4(%edx)
     1fe:	83 f8 05             	cmp    $0x5,%eax
     201:	75 cd                	jne    0x1d0
     203:	c7 05 44 6c 00 00 00 	movl   $0x0,0x6c44
     20a:	00 00 00 
     20d:	31 c0                	xor    %eax,%eax
     20f:	c7 05 50 6c 00 00 e8 	movl   $0x3e8,0x6c50
     216:	03 00 00 
     219:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
     220:	c7 04 85 58 6c 00 00 	movl   $0x0,0x6c58(,%eax,4)
     227:	00 00 00 00 
     22b:	83 c1 18             	add    $0x18,%ecx
     22e:	c7 04 85 6c 6c 00 00 	movl   $0x64,0x6c6c(,%eax,4)
     235:	64 00 00 00 
     239:	c7 04 85 80 6c 00 00 	movl   $0x3e8,0x6c80(,%eax,4)
     240:	e8 03 00 00 
     244:	83 c0 01             	add    $0x1,%eax
     247:	c7 41 7c e8 03 00 00 	movl   $0x3e8,0x7c(%ecx)
     24e:	83 f8 05             	cmp    $0x5,%eax
     251:	75 cd                	jne    0x220
     253:	b8 36 00 00 00       	mov    $0x36,%eax
     258:	e6 43                	out    %al,$0x43
     25a:	b8 9b ff ff ff       	mov    $0xffffff9b,%eax
     25f:	e6 40                	out    %al,$0x40
     261:	b8 2e 00 00 00       	mov    $0x2e,%eax
     266:	e6 40                	out    %al,$0x40
     268:	e8 63 03 00 00       	call   0x5d0
     26d:	e8 7e 2e 00 00       	call   0x30f0
     272:	a3 2c 6e 00 00       	mov    %eax,0x6e2c
     277:	b8 00 80 0b 00       	mov    $0xb8000,%eax
     27c:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
     280:	ba 20 1f 00 00       	mov    $0x1f20,%edx
     285:	b9 20 1f 00 00       	mov    $0x1f20,%ecx
     28a:	83 c0 04             	add    $0x4,%eax
     28d:	66 89 50 fc          	mov    %dx,-0x4(%eax)
     291:	66 89 48 fe          	mov    %cx,-0x2(%eax)
     295:	3d a0 8f 0b 00       	cmp    $0xb8fa0,%eax
     29a:	75 e4                	jne    0x280
     29c:	b9 d5 57 00 00       	mov    $0x57d5,%ecx
     2a1:	ba 00 80 0b 00       	mov    $0xb8000,%edx
     2a6:	b8 54 00 00 00       	mov    $0x54,%eax
     2ab:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
     2af:	90                   	nop
     2b0:	80 cc 1f             	or     $0x1f,%ah
     2b3:	83 c2 02             	add    $0x2,%edx
     2b6:	83 c1 01             	add    $0x1,%ecx
     2b9:	66 89 42 fe          	mov    %ax,-0x2(%edx)
     2bd:	66 0f be 41 ff       	movsbw -0x1(%ecx),%ax
     2c2:	84 c0                	test   %al,%al
     2c4:	75 ea                	jne    0x2b0
     2c6:	e8 d5 31 00 00       	call   0x34a0
     2cb:	e8 30 35 00 00       	call   0x3800
     2d0:	c7 05 60 b0 00 00 00 	movl   $0x0,0xb060
     2d7:	00 00 00 
     2da:	c7 05 60 59 00 00 78 	movl   $0x78,0x5960
     2e1:	00 00 00 
     2e4:	e8 07 3e 00 00       	call   0x40f0
     2e9:	e8 32 37 00 00       	call   0x3a20
     2ee:	83 ec 04             	sub    $0x4,%esp
     2f1:	c7 05 3c 6c 00 00 00 	movl   $0x0,0x6c3c
     2f8:	00 00 00 
     2fb:	68 00 a0 00 00       	push   $0xa000
     300:	68 50 2b 00 00       	push   $0x2b50
     305:	6a 01                	push   $0x1
     307:	c7 05 e0 6b 00 00 00 	movl   $0x0,0x6be0
     30e:	00 00 00 
     311:	e8 aa 36 00 00       	call   0x39c0
     316:	6a 0e                	push   $0xe
     318:	6a 00                	push   $0x0
     31a:	6a 00                	push   $0x0
     31c:	68 f7 55 00 00       	push   $0x55f7
     321:	e8 6a 17 00 00       	call   0x1a90
     326:	83 c4 20             	add    $0x20,%esp
     329:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
     330:	f4                   	hlt
     331:	eb fd                	jmp    0x330
     333:	66 90                	xchg   %ax,%ax
     335:	66 90                	xchg   %ax,%ax
     337:	66 90                	xchg   %ax,%ax
     339:	66 90                	xchg   %ax,%ax
     33b:	66 90                	xchg   %ax,%ax
     33d:	66 90                	xchg   %ax,%ax
     33f:	90                   	nop
     340:	55                   	push   %ebp
     341:	89 c1                	mov    %eax,%ecx
     343:	89 e5                	mov    %esp,%ebp
     345:	57                   	push   %edi
     346:	56                   	push   %esi
     347:	53                   	push   %ebx
     348:	89 d3                	mov    %edx,%ebx
     34a:	83 ec 08             	sub    $0x8,%esp
     34d:	85 c0                	test   %eax,%eax
     34f:	0f 88 ab 00 00 00    	js     0x400
     355:	b8 00 00 00 00       	mov    $0x0,%eax
     35a:	0f 84 90 00 00 00    	je     0x3f0
     360:	89 45 ec             	mov    %eax,-0x14(%ebp)
     363:	31 f6                	xor    %esi,%esi
     365:	8d 76 00             	lea    0x0(%esi),%esi
     368:	b8 67 66 66 66       	mov    $0x66666667,%eax
     36d:	89 75 f0             	mov    %esi,-0x10(%ebp)
     370:	83 c6 01             	add    $0x1,%esi
     373:	f7 e9                	imul   %ecx
     375:	89 c8                	mov    %ecx,%eax
     377:	89 f7                	mov    %esi,%edi
     379:	c1 f8 1f             	sar    $0x1f,%eax
     37c:	c1 fa 02             	sar    $0x2,%edx
     37f:	29 c2                	sub    %eax,%edx
     381:	8d 04 92             	lea    (%edx,%edx,4),%eax
     384:	01 c0                	add    %eax,%eax
     386:	29 c1                	sub    %eax,%ecx
     388:	83 c1 30             	add    $0x30,%ecx
     38b:	88 4c 33 ff          	mov    %cl,-0x1(%ebx,%esi,1)
     38f:	89 d1                	mov    %edx,%ecx
     391:	85 d2                	test   %edx,%edx
     393:	75 d3                	jne    0x368
     395:	8b 45 ec             	mov    -0x14(%ebp),%eax
     398:	85 c0                	test   %eax,%eax
     39a:	74 44                	je     0x3e0
     39c:	8b 45 f0             	mov    -0x10(%ebp),%eax
     39f:	c6 04 33 2d          	movb   $0x2d,(%ebx,%esi,1)
     3a3:	8d 78 02             	lea    0x2(%eax),%edi
     3a6:	c6 44 03 02 00       	movb   $0x0,0x2(%ebx,%eax,1)
     3ab:	89 fe                	mov    %edi,%esi
     3ad:	d1 fe                	sar    %esi
     3af:	8d 44 1f ff          	lea    -0x1(%edi,%ebx,1),%eax
     3b3:	01 de                	add    %ebx,%esi
     3b5:	8d 76 00             	lea    0x0(%esi),%esi
     3b8:	0f b6 08             	movzbl (%eax),%ecx
     3bb:	0f b6 13             	movzbl (%ebx),%edx
     3be:	83 c3 01             	add    $0x1,%ebx
     3c1:	83 e8 01             	sub    $0x1,%eax
     3c4:	88 4b ff             	mov    %cl,-0x1(%ebx)
     3c7:	88 50 01             	mov    %dl,0x1(%eax)
     3ca:	39 de                	cmp    %ebx,%esi
     3cc:	75 ea                	jne    0x3b8
     3ce:	83 c4 08             	add    $0x8,%esp
     3d1:	5b                   	pop    %ebx
     3d2:	5e                   	pop    %esi
     3d3:	5f                   	pop    %edi
     3d4:	5d                   	pop    %ebp
     3d5:	c3                   	ret
     3d6:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
     3dd:	8d 76 00             	lea    0x0(%esi),%esi
     3e0:	c6 04 33 00          	movb   $0x0,(%ebx,%esi,1)
     3e4:	d1 fe                	sar    %esi
     3e6:	75 c7                	jne    0x3af
     3e8:	eb e4                	jmp    0x3ce
     3ea:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
     3f0:	c6 02 00             	movb   $0x0,(%edx)
     3f3:	83 c4 08             	add    $0x8,%esp
     3f6:	5b                   	pop    %ebx
     3f7:	5e                   	pop    %esi
     3f8:	5f                   	pop    %edi
     3f9:	5d                   	pop    %ebp
     3fa:	c3                   	ret
     3fb:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
     3ff:	90                   	nop
     400:	f7 d9                	neg    %ecx
     402:	b8 01 00 00 00       	mov    $0x1,%eax
     407:	e9 54 ff ff ff       	jmp    0x360
     40c:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
     410:	55                   	push   %ebp
     411:	89 e5                	mov    %esp,%ebp
     413:	57                   	push   %edi
     414:	89 c7                	mov    %eax,%edi
     416:	8d 04 00             	lea    (%eax,%eax,1),%eax
     419:	56                   	push   %esi
     41a:	89 d6                	mov    %edx,%esi
     41c:	8d 14 d5 00 00 00 00 	lea    0x0(,%edx,8),%edx
     423:	53                   	push   %ebx
     424:	8d 0c 32             	lea    (%edx,%esi,1),%ecx
     427:	8d 0c cd c0 6a 00 00 	lea    0x6ac0(,%ecx,8),%ecx
     42e:	83 ec 1c             	sub    $0x1c,%esp
     431:	89 45 e4             	mov    %eax,-0x1c(%ebp)
     434:	01 f8                	add    %edi,%eax
     436:	89 7d e0             	mov    %edi,-0x20(%ebp)
     439:	c1 e0 05             	shl    $0x5,%eax
     43c:	89 55 dc             	mov    %edx,-0x24(%ebp)
     43f:	8b 98 f4 6b 00 00    	mov    0x6bf4(%eax),%ebx
     445:	31 c0                	xor    %eax,%eax
     447:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
     44e:	66 90                	xchg   %ax,%ax
     450:	0f b6 14 03          	movzbl (%ebx,%eax,1),%edx
     454:	8d 3c 01             	lea    (%ecx,%eax,1),%edi
     457:	83 c0 01             	add    $0x1,%eax
     45a:	88 17                	mov    %dl,(%edi)
     45c:	83 f8 08             	cmp    $0x8,%eax
     45f:	75 ef                	jne    0x450
     461:	8b 7d e0             	mov    -0x20(%ebp),%edi
     464:	8b 45 e4             	mov    -0x1c(%ebp),%eax
     467:	c7 41 08 01 00 00 00 	movl   $0x1,0x8(%ecx)
     46e:	31 db                	xor    %ebx,%ebx
     470:	8b 55 dc             	mov    -0x24(%ebp),%edx
     473:	89 75 e4             	mov    %esi,-0x1c(%ebp)
     476:	01 f8                	add    %edi,%eax
     478:	89 c7                	mov    %eax,%edi
     47a:	01 f2                	add    %esi,%edx
     47c:	c1 e7 05             	shl    $0x5,%edi
     47f:	89 55 e0             	mov    %edx,-0x20(%ebp)
     482:	8b 87 e8 6b 00 00    	mov    0x6be8(%edi),%eax
     488:	8d 3c 12             	lea    (%edx,%edx,1),%edi
     48b:	c1 e0 07             	shl    $0x7,%eax
     48e:	05 a0 6c 00 00       	add    $0x6ca0,%eax
     493:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
     497:	90                   	nop
     498:	8b 30                	mov    (%eax),%esi
     49a:	8d 0c 1f             	lea    (%edi,%ebx,1),%ecx
     49d:	8b 50 0c             	mov    0xc(%eax),%edx
     4a0:	83 c3 01             	add    $0x1,%ebx
     4a3:	83 c0 18             	add    $0x18,%eax
     4a6:	89 34 8d cc 6a 00 00 	mov    %esi,0x6acc(,%ecx,4)
     4ad:	8d 71 08             	lea    0x8(%ecx),%esi
     4b0:	83 c1 0c             	add    $0xc,%ecx
     4b3:	89 14 b5 c0 6a 00 00 	mov    %edx,0x6ac0(,%esi,4)
     4ba:	8b 70 fc             	mov    -0x4(%eax),%esi
     4bd:	89 34 8d c4 6a 00 00 	mov    %esi,0x6ac4(,%ecx,4)
     4c4:	83 fb 05             	cmp    $0x5,%ebx
     4c7:	75 cf                	jne    0x498
     4c9:	8b 75 e4             	mov    -0x1c(%ebp),%esi
     4cc:	8b 55 e0             	mov    -0x20(%ebp),%edx
     4cf:	a1 2c 6e 00 00       	mov    0x6e2c,%eax
     4d4:	85 c0                	test   %eax,%eax
     4d6:	75 28                	jne    0x500
     4d8:	a1 2c 6e 00 00       	mov    0x6e2c,%eax
     4dd:	83 f8 01             	cmp    $0x1,%eax
     4e0:	19 c0                	sbb    %eax,%eax
     4e2:	83 e0 09             	and    $0x9,%eax
     4e5:	66 05 44 2f          	add    $0x2f44,%ax
     4e9:	66 a3 9e 80 0b 00    	mov    %ax,0xb809e
     4ef:	8d 65 f4             	lea    -0xc(%ebp),%esp
     4f2:	5b                   	pop    %ebx
     4f3:	5e                   	pop    %esi
     4f4:	5f                   	pop    %edi
     4f5:	5d                   	pop    %ebp
     4f6:	c3                   	ret
     4f7:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
     4fe:	66 90                	xchg   %ax,%ax
     500:	83 ec 08             	sub    $0x8,%esp
     503:	8d 04 d5 c0 6a 00 00 	lea    0x6ac0(,%edx,8),%eax
     50a:	50                   	push   %eax
     50b:	8d 46 42             	lea    0x42(%esi),%eax
     50e:	50                   	push   %eax
     50f:	e8 5c 2c 00 00       	call   0x3170
     514:	83 c4 10             	add    $0x10,%esp
     517:	eb bf                	jmp    0x4d8
     519:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
     520:	c7 05 3a 59 00 00 40 	movl   $0x5940,0x593a
     527:	59 00 00 
     52a:	ba 38 59 00 00       	mov    $0x5938,%edx
     52f:	0f 01 12             	lgdtl  (%edx)
     532:	ea 39 15 00 00 08 00 	ljmp   $0x8,$0x1539
     539:	66 b8 10 00          	mov    $0x10,%ax
     53d:	8e d8                	mov    %eax,%ds
     53f:	8e c0                	mov    %eax,%es
     541:	8e e0                	mov    %eax,%fs
     543:	8e e8                	mov    %eax,%gs
     545:	8e d0                	mov    %eax,%ss
     547:	c3                   	ret
     548:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
     54f:	90                   	nop
     550:	55                   	push   %ebp
     551:	89 e5                	mov    %esp,%ebp
     553:	8b 45 08             	mov    0x8(%ebp),%eax
     556:	8b 4d 10             	mov    0x10(%ebp),%ecx
     559:	8b 55 0c             	mov    0xc(%ebp),%edx
     55c:	66 89 0c c5 22 60 00 	mov    %cx,0x6022(,%eax,8)
     563:	00 
     564:	8b 4d 14             	mov    0x14(%ebp),%ecx
     567:	66 89 14 c5 20 60 00 	mov    %dx,0x6020(,%eax,8)
     56e:	00 
     56f:	c1 ea 10             	shr    $0x10,%edx
     572:	5d                   	pop    %ebp
     573:	c6 04 c5 24 60 00 00 	movb   $0x0,0x6024(,%eax,8)
     57a:	00 
     57b:	88 0c c5 25 60 00 00 	mov    %cl,0x6025(,%eax,8)
     582:	66 89 14 c5 26 60 00 	mov    %dx,0x6026(,%eax,8)
     589:	00 
     58a:	c3                   	ret
     58b:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
     58f:	90                   	nop
     590:	b8 45 4f 00 00       	mov    $0x4f45,%eax
     595:	ba 58 4f 00 00       	mov    $0x4f58,%edx
     59a:	b9 43 4f 00 00       	mov    $0x4f43,%ecx
     59f:	66 a3 00 80 0b 00    	mov    %ax,0xb8000
     5a5:	b8 21 4f 00 00       	mov    $0x4f21,%eax
     5aa:	66 89 15 02 80 0b 00 	mov    %dx,0xb8002
     5b1:	66 89 0d 04 80 0b 00 	mov    %cx,0xb8004
     5b8:	66 a3 06 80 0b 00    	mov    %ax,0xb8006
     5be:	66 90                	xchg   %ax,%ax
     5c0:	f4                   	hlt
     5c1:	eb fd                	jmp    0x5c0
     5c3:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
     5ca:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
     5d0:	55                   	push   %ebp
     5d1:	b8 ff 07 00 00       	mov    $0x7ff,%eax
     5d6:	ba 50 49 00 00       	mov    $0x4950,%edx
     5db:	0f b7 0d 58 58 00 00 	movzwl 0x5858,%ecx
     5e2:	66 a3 04 60 00 00    	mov    %ax,0x6004
     5e8:	89 d0                	mov    %edx,%eax
     5ea:	c7 05 06 60 00 00 20 	movl   $0x6020,0x6006
     5f1:	60 00 00 
     5f4:	c1 e8 10             	shr    $0x10,%eax
     5f7:	89 e5                	mov    %esp,%ebp
     5f9:	53                   	push   %ebx
     5fa:	89 c3                	mov    %eax,%ebx
     5fc:	31 c0                	xor    %eax,%eax
     5fe:	66 90                	xchg   %ax,%ax
     600:	66 89 14 c5 20 60 00 	mov    %dx,0x6020(,%eax,8)
     607:	00 
     608:	66 c7 04 c5 22 60 00 	movw   $0x8,0x6022(,%eax,8)
     60f:	00 08 00 
     612:	66 89 0c c5 24 60 00 	mov    %cx,0x6024(,%eax,8)
     619:	00 
     61a:	66 89 1c c5 26 60 00 	mov    %bx,0x6026(,%eax,8)
     621:	00 
     622:	83 c0 01             	add    $0x1,%eax
     625:	83 f8 20             	cmp    $0x20,%eax
     628:	75 d6                	jne    0x600
     62a:	b8 11 00 00 00       	mov    $0x11,%eax
     62f:	e6 20                	out    %al,$0x20
     631:	e6 a0                	out    %al,$0xa0
     633:	b8 20 00 00 00       	mov    $0x20,%eax
     638:	e6 21                	out    %al,$0x21
     63a:	b8 28 00 00 00       	mov    $0x28,%eax
     63f:	e6 a1                	out    %al,$0xa1
     641:	b8 04 00 00 00       	mov    $0x4,%eax
     646:	e6 21                	out    %al,$0x21
     648:	b8 02 00 00 00       	mov    $0x2,%eax
     64d:	e6 a1                	out    %al,$0xa1
     64f:	b8 01 00 00 00       	mov    $0x1,%eax
     654:	e6 21                	out    %al,$0x21
     656:	e6 a1                	out    %al,$0xa1
     658:	b8 fc ff ff ff       	mov    $0xfffffffc,%eax
     65d:	e6 21                	out    %al,$0x21
     65f:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
     664:	e6 a1                	out    %al,$0xa1
     666:	c7 05 22 61 00 00 08 	movl   $0x8e000008,0x6122
     66d:	00 00 8e 
     670:	b8 20 49 00 00       	mov    $0x4920,%eax
     675:	66 a3 20 61 00 00    	mov    %ax,0x6120
     67b:	c1 e8 10             	shr    $0x10,%eax
     67e:	66 a3 26 61 00 00    	mov    %ax,0x6126
     684:	b8 2c 49 00 00       	mov    $0x492c,%eax
     689:	66 a3 28 61 00 00    	mov    %ax,0x6128
     68f:	c1 e8 10             	shr    $0x10,%eax
     692:	66 a3 2e 61 00 00    	mov    %ax,0x612e
     698:	b8 04 60 00 00       	mov    $0x6004,%eax
     69d:	c7 05 2a 61 00 00 08 	movl   $0x8e000008,0x612a
     6a4:	00 00 8e 
     6a7:	0f 01 18             	lidtl  (%eax)
     6aa:	fb                   	sti
     6ab:	8b 5d fc             	mov    -0x4(%ebp),%ebx
     6ae:	c9                   	leave
     6af:	c3                   	ret
     6b0:	a1 28 59 00 00       	mov    0x5928,%eax
     6b5:	a3 20 6e 00 00       	mov    %eax,0x6e20
     6ba:	8d 90 00 4c 1d 00    	lea    0x1d4c00(%eax),%edx
     6c0:	c7 00 44 00 00 00    	movl   $0x44,(%eax)
     6c6:	83 c0 08             	add    $0x8,%eax
     6c9:	c7 40 fc 44 00 00 00 	movl   $0x44,-0x4(%eax)
     6d0:	39 d0                	cmp    %edx,%eax
     6d2:	75 ec                	jne    0x6c0
     6d4:	c3                   	ret
     6d5:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
     6dc:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
     6e0:	55                   	push   %ebp
     6e1:	31 d2                	xor    %edx,%edx
     6e3:	b8 de 34 12 00       	mov    $0x1234de,%eax
     6e8:	89 e5                	mov    %esp,%ebp
     6ea:	f7 75 08             	divl   0x8(%ebp)
     6ed:	89 c1                	mov    %eax,%ecx
     6ef:	b8 36 00 00 00       	mov    $0x36,%eax
     6f4:	e6 43                	out    %al,$0x43
     6f6:	89 c8                	mov    %ecx,%eax
     6f8:	e6 40                	out    %al,$0x40
     6fa:	89 c8                	mov    %ecx,%eax
     6fc:	c1 e8 08             	shr    $0x8,%eax
     6ff:	e6 40                	out    %al,$0x40
     701:	5d                   	pop    %ebp
     702:	c3                   	ret
     703:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
     70a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
     710:	55                   	push   %ebp
     711:	89 e5                	mov    %esp,%ebp
     713:	8b 45 08             	mov    0x8(%ebp),%eax
     716:	8b 55 0c             	mov    0xc(%ebp),%edx
     719:	85 c0                	test   %eax,%eax
     71b:	74 0b                	je     0x728
     71d:	5d                   	pop    %ebp
     71e:	e9 1d fc ff ff       	jmp    0x340
     723:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
     727:	90                   	nop
     728:	b8 30 00 00 00       	mov    $0x30,%eax
     72d:	66 89 02             	mov    %ax,(%edx)
     730:	5d                   	pop    %ebp
     731:	c3                   	ret
     732:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
     739:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
     740:	55                   	push   %ebp
     741:	89 e5                	mov    %esp,%ebp
     743:	57                   	push   %edi
     744:	bf 52 86 0b 00       	mov    $0xb8652,%edi
     749:	56                   	push   %esi
     74a:	53                   	push   %ebx
     74b:	83 ec 3c             	sub    $0x3c,%esp
     74e:	c7 45 d0 00 00 00 00 	movl   $0x0,-0x30(%ebp)
     755:	8b 45 d0             	mov    -0x30(%ebp),%eax
     758:	c7 45 cc a0 6c 00 00 	movl   $0x6ca0,-0x34(%ebp)
     75f:	c7 45 c4 00 00 00 00 	movl   $0x0,-0x3c(%ebp)
     766:	39 05 e8 6b 00 00    	cmp    %eax,0x6be8
     76c:	0f 84 51 02 00 00    	je     0x9c3
     772:	39 05 48 6c 00 00    	cmp    %eax,0x6c48
     778:	be 40 6c 00 00       	mov    $0x6c40,%esi
     77d:	b8 00 00 00 00       	mov    $0x0,%eax
     782:	0f 44 c6             	cmove  %esi,%eax
     785:	8b 75 cc             	mov    -0x34(%ebp),%esi
     788:	8b 4d d0             	mov    -0x30(%ebp),%ecx
     78b:	8b 56 04             	mov    0x4(%esi),%edx
     78e:	03 56 1c             	add    0x1c(%esi),%edx
     791:	03 56 34             	add    0x34(%esi),%edx
     794:	03 56 4c             	add    0x4c(%esi),%edx
     797:	03 56 64             	add    0x64(%esi),%edx
     79a:	01 55 c4             	add    %edx,-0x3c(%ebp)
     79d:	89 d3                	mov    %edx,%ebx
     79f:	2b 1c 8d 24 6e 00 00 	sub    0x6e24(,%ecx,4),%ebx
     7a6:	89 14 8d 24 6e 00 00 	mov    %edx,0x6e24(,%ecx,4)
     7ad:	8d 0c 9b             	lea    (%ebx,%ebx,4),%ecx
     7b0:	89 5d c0             	mov    %ebx,-0x40(%ebp)
     7b3:	01 c9                	add    %ecx,%ecx
     7b5:	89 50 54             	mov    %edx,0x54(%eax)
     7b8:	89 4d bc             	mov    %ecx,-0x44(%ebp)
     7bb:	89 48 58             	mov    %ecx,0x58(%eax)
     7be:	83 fb 07             	cmp    $0x7,%ebx
     7c1:	7e 4d                	jle    0x810
     7c3:	c6 46 78 01          	movb   $0x1,0x78(%esi)
     7c7:	89 f2                	mov    %esi,%edx
     7c9:	8d 5e 78             	lea    0x78(%esi),%ebx
     7cc:	8b 0a                	mov    (%edx),%ecx
     7ce:	81 f9 c8 00 00 00    	cmp    $0xc8,%ecx
     7d4:	7e 08                	jle    0x7de
     7d6:	81 e9 c8 00 00 00    	sub    $0xc8,%ecx
     7dc:	89 0a                	mov    %ecx,(%edx)
     7de:	83 c2 18             	add    $0x18,%edx
     7e1:	39 da                	cmp    %ebx,%edx
     7e3:	75 e7                	jne    0x7cc
     7e5:	be 20 4e 00 00       	mov    $0x4e20,%esi
     7ea:	b9 3a 4e 00 00       	mov    $0x4e3a,%ecx
     7ef:	ba 52 00 00 00       	mov    $0x52,%edx
     7f4:	c7 45 b8 e7 54 00 00 	movl   $0x54e7,-0x48(%ebp)
     7fb:	66 89 75 ca          	mov    %si,-0x36(%ebp)
     7ff:	be 00 4e 00 00       	mov    $0x4e00,%esi
     804:	66 89 4d c8          	mov    %cx,-0x38(%ebp)
     808:	eb 30                	jmp    0x83a
     80a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
     810:	8b 75 cc             	mov    -0x34(%ebp),%esi
     813:	bb 20 1f 00 00       	mov    $0x1f20,%ebx
     818:	ba 46 00 00 00       	mov    $0x46,%edx
     81d:	c7 45 b8 e0 54 00 00 	movl   $0x54e0,-0x48(%ebp)
     824:	66 89 5d ca          	mov    %bx,-0x36(%ebp)
     828:	c6 46 78 00          	movb   $0x0,0x78(%esi)
     82c:	be 3a 1f 00 00       	mov    $0x1f3a,%esi
     831:	66 89 75 c8          	mov    %si,-0x38(%ebp)
     835:	be 00 1f 00 00       	mov    $0x1f00,%esi
     83a:	8b 48 14             	mov    0x14(%eax),%ecx
     83d:	8d 9f 6e fd ff ff    	lea    -0x292(%edi),%ebx
     843:	66 0f be 01          	movsbw (%ecx),%ax
     847:	83 c1 01             	add    $0x1,%ecx
     84a:	84 c0                	test   %al,%al
     84c:	74 17                	je     0x865
     84e:	66 90                	xchg   %ax,%ax
     850:	09 f0                	or     %esi,%eax
     852:	83 c3 02             	add    $0x2,%ebx
     855:	83 c1 01             	add    $0x1,%ecx
     858:	66 89 43 fe          	mov    %ax,-0x2(%ebx)
     85c:	66 0f be 41 ff       	movsbw -0x1(%ecx),%ax
     861:	84 c0                	test   %al,%al
     863:	75 eb                	jne    0x850
     865:	0f b7 45 c8          	movzwl -0x38(%ebp),%eax
     869:	8d 8f 7a fd ff ff    	lea    -0x286(%edi),%ecx
     86f:	66 89 87 76 fd ff ff 	mov    %ax,-0x28a(%edi)
     876:	0f b7 45 ca          	movzwl -0x36(%ebp),%eax
     87a:	66 89 87 78 fd ff ff 	mov    %ax,-0x288(%edi)
     881:	8b 45 b8             	mov    -0x48(%ebp),%eax
     884:	83 c0 01             	add    $0x1,%eax
     887:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
     88e:	66 90                	xchg   %ax,%ax
     890:	09 f2                	or     %esi,%edx
     892:	83 c1 02             	add    $0x2,%ecx
     895:	83 c0 01             	add    $0x1,%eax
     898:	66 89 51 fe          	mov    %dx,-0x2(%ecx)
     89c:	66 0f be 50 ff       	movsbw -0x1(%eax),%dx
     8a1:	84 d2                	test   %dl,%dl
     8a3:	75 eb                	jne    0x890
     8a5:	0f b7 45 ca          	movzwl -0x36(%ebp),%eax
     8a9:	8b 5d c0             	mov    -0x40(%ebp),%ebx
     8ac:	66 89 87 84 fd ff ff 	mov    %ax,-0x27c(%edi)
     8b3:	66 89 87 86 fd ff ff 	mov    %ax,-0x27a(%edi)
     8ba:	85 db                	test   %ebx,%ebx
     8bc:	0f 84 66 02 00 00    	je     0xb28
     8c2:	8b 45 bc             	mov    -0x44(%ebp),%eax
     8c5:	8d 55 d6             	lea    -0x2a(%ebp),%edx
     8c8:	e8 73 fa ff ff       	call   0x340
     8cd:	66 0f be 45 d6       	movsbw -0x2a(%ebp),%ax
     8d2:	8d 9f 0e fe ff ff    	lea    -0x1f2(%edi),%ebx
     8d8:	b9 ef 54 00 00       	mov    $0x54ef,%ecx
     8dd:	ba 53 00 00 00       	mov    $0x53,%edx
     8e2:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
     8e8:	80 ce 07             	or     $0x7,%dh
     8eb:	83 c3 02             	add    $0x2,%ebx
     8ee:	83 c1 01             	add    $0x1,%ecx
     8f1:	66 89 53 fe          	mov    %dx,-0x2(%ebx)
     8f5:	66 0f be 51 ff       	movsbw -0x1(%ecx),%dx
     8fa:	84 d2                	test   %dl,%dl
     8fc:	75 ea                	jne    0x8e8
     8fe:	8d 8f 18 fe ff ff    	lea    -0x1e8(%edi),%ecx
     904:	8d 55 d7             	lea    -0x29(%ebp),%edx
     907:	84 c0                	test   %al,%al
     909:	74 1b                	je     0x926
     90b:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
     90f:	90                   	nop
     910:	80 cc 0e             	or     $0xe,%ah
     913:	83 c1 02             	add    $0x2,%ecx
     916:	83 c2 01             	add    $0x1,%edx
     919:	66 89 41 fe          	mov    %ax,-0x2(%ecx)
     91d:	66 0f be 42 ff       	movsbw -0x1(%edx),%ax
     922:	84 c0                	test   %al,%al
     924:	75 ea                	jne    0x910
     926:	c7 87 1e fe ff ff 20 	movl   $0x7200720,-0x1e2(%edi)
     92d:	07 20 07 
     930:	8b 45 cc             	mov    -0x34(%ebp),%eax
     933:	8b 40 14             	mov    0x14(%eax),%eax
     936:	85 c0                	test   %eax,%eax
     938:	0f 84 d2 01 00 00    	je     0xb10
     93e:	8d 55 e0             	lea    -0x20(%ebp),%edx
     941:	e8 fa f9 ff ff       	call   0x340
     946:	66 0f be 45 e0       	movsbw -0x20(%ebp),%ax
     94b:	8d 5f ee             	lea    -0x12(%edi),%ebx
     94e:	b9 f5 54 00 00       	mov    $0x54f5,%ecx
     953:	ba 54 00 00 00       	mov    $0x54,%edx
     958:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
     95f:	90                   	nop
     960:	80 ce 07             	or     $0x7,%dh
     963:	83 c3 02             	add    $0x2,%ebx
     966:	83 c1 01             	add    $0x1,%ecx
     969:	66 89 53 fe          	mov    %dx,-0x2(%ebx)
     96d:	66 0f be 51 ff       	movsbw -0x1(%ecx),%dx
     972:	84 d2                	test   %dl,%dl
     974:	75 ea                	jne    0x960
     976:	8d 4f f8             	lea    -0x8(%edi),%ecx
     979:	8d 55 e1             	lea    -0x1f(%ebp),%edx
     97c:	84 c0                	test   %al,%al
     97e:	74 16                	je     0x996
     980:	80 cc 0b             	or     $0xb,%ah
     983:	83 c1 02             	add    $0x2,%ecx
     986:	83 c2 01             	add    $0x1,%edx
     989:	66 89 41 fe          	mov    %ax,-0x2(%ecx)
     98d:	66 0f be 42 ff       	movsbw -0x1(%edx),%ax
     992:	84 c0                	test   %al,%al
     994:	75 ea                	jne    0x980
     996:	b8 20 07 00 00       	mov    $0x720,%eax
     99b:	83 6d cc 80          	subl   $0xffffff80,-0x34(%ebp)
     99f:	83 c7 50             	add    $0x50,%edi
     9a2:	66 89 47 b0          	mov    %ax,-0x50(%edi)
     9a6:	8b 45 d0             	mov    -0x30(%ebp),%eax
     9a9:	85 c0                	test   %eax,%eax
     9ab:	75 23                	jne    0x9d0
     9ad:	c7 45 d0 01 00 00 00 	movl   $0x1,-0x30(%ebp)
     9b4:	8b 45 d0             	mov    -0x30(%ebp),%eax
     9b7:	39 05 e8 6b 00 00    	cmp    %eax,0x6be8
     9bd:	0f 85 af fd ff ff    	jne    0x772
     9c3:	b8 e0 6b 00 00       	mov    $0x6be0,%eax
     9c8:	e9 b8 fd ff ff       	jmp    0x785
     9cd:	8d 76 00             	lea    0x0(%esi),%esi
     9d0:	8b 45 c4             	mov    -0x3c(%ebp),%eax
     9d3:	85 c0                	test   %eax,%eax
     9d5:	0f 84 65 01 00 00    	je     0xb40
     9db:	8b 45 c4             	mov    -0x3c(%ebp),%eax
     9de:	8d 4d d6             	lea    -0x2a(%ebp),%ecx
     9e1:	89 ca                	mov    %ecx,%edx
     9e3:	e8 58 f9 ff ff       	call   0x340
     9e8:	66 0f be 45 d6       	movsbw -0x2a(%ebp),%ax
     9ed:	bb fb 54 00 00       	mov    $0x54fb,%ebx
     9f2:	b9 80 82 0b 00       	mov    $0xb8280,%ecx
     9f7:	ba 54 00 00 00       	mov    $0x54,%edx
     9fc:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
     a00:	80 ce 0f             	or     $0xf,%dh
     a03:	83 c1 02             	add    $0x2,%ecx
     a06:	83 c3 01             	add    $0x1,%ebx
     a09:	66 89 51 fe          	mov    %dx,-0x2(%ecx)
     a0d:	66 0f be 53 ff       	movsbw -0x1(%ebx),%dx
     a12:	84 d2                	test   %dl,%dl
     a14:	75 ea                	jne    0xa00
     a16:	31 d2                	xor    %edx,%edx
     a18:	8d 4d d6             	lea    -0x2a(%ebp),%ecx
     a1b:	84 c0                	test   %al,%al
     a1d:	74 18                	je     0xa37
     a1f:	90                   	nop
     a20:	80 cc 0e             	or     $0xe,%ah
     a23:	66 89 84 12 a6 82 0b 	mov    %ax,0xb82a6(%edx,%edx,1)
     a2a:	00 
     a2b:	83 c2 01             	add    $0x1,%edx
     a2e:	66 0f be 04 11       	movsbw (%ecx,%edx,1),%ax
     a33:	84 c0                	test   %al,%al
     a35:	75 e9                	jne    0xa20
     a37:	8b 35 e8 6b 00 00    	mov    0x6be8,%esi
     a3d:	8b 1d 48 6c 00 00    	mov    0x6c48,%ebx
     a43:	85 f6                	test   %esi,%esi
     a45:	0f 84 0f 01 00 00    	je     0xb5a
     a4b:	85 db                	test   %ebx,%ebx
     a4d:	0f 85 13 01 00 00    	jne    0xb66
     a53:	8b 45 d0             	mov    -0x30(%ebp),%eax
     a56:	c6 45 e3 54          	movb   $0x54,-0x1d(%ebp)
     a5a:	bf 30 3a 00 00       	mov    $0x3a30,%edi
     a5f:	8d 55 e1             	lea    -0x1f(%ebp),%edx
     a62:	66 89 7d e1          	mov    %di,-0x1f(%ebp)
     a66:	b9 a0 85 0b 00       	mov    $0xb85a0,%ecx
     a6b:	89 d7                	mov    %edx,%edi
     a6d:	8d 04 40             	lea    (%eax,%eax,2),%eax
     a70:	c1 e0 05             	shl    $0x5,%eax
     a73:	8b 80 e0 6b 00 00    	mov    0x6be0(%eax),%eax
     a79:	83 c0 30             	add    $0x30,%eax
     a7c:	88 45 e4             	mov    %al,-0x1c(%ebp)
     a7f:	b8 20 00 00 00       	mov    $0x20,%eax
     a84:	66 89 45 e5          	mov    %ax,-0x1b(%ebp)
     a88:	b8 50 00 00 00       	mov    $0x50,%eax
     a8d:	8d 76 00             	lea    0x0(%esi),%esi
     a90:	80 cc 0f             	or     $0xf,%ah
     a93:	83 c1 02             	add    $0x2,%ecx
     a96:	83 c7 01             	add    $0x1,%edi
     a99:	66 89 41 fe          	mov    %ax,-0x2(%ecx)
     a9d:	66 0f be 47 ff       	movsbw -0x1(%edi),%ax
     aa2:	84 c0                	test   %al,%al
     aa4:	75 ea                	jne    0xa90
     aa6:	83 fe 01             	cmp    $0x1,%esi
     aa9:	0f 84 a4 00 00 00    	je     0xb53
     aaf:	83 fb 01             	cmp    $0x1,%ebx
     ab2:	0f 85 7a 39 00 00    	jne    0x4432
     ab8:	b8 31 3a 00 00       	mov    $0x3a31,%eax
     abd:	c6 45 e3 54          	movb   $0x54,-0x1d(%ebp)
     ac1:	b9 20 00 00 00       	mov    $0x20,%ecx
     ac6:	66 89 45 e1          	mov    %ax,-0x1f(%ebp)
     aca:	8d 04 5b             	lea    (%ebx,%ebx,2),%eax
     acd:	c1 e0 05             	shl    $0x5,%eax
     ad0:	66 89 4d e5          	mov    %cx,-0x1b(%ebp)
     ad4:	b9 b4 85 0b 00       	mov    $0xb85b4,%ecx
     ad9:	8b 80 e0 6b 00 00    	mov    0x6be0(%eax),%eax
     adf:	83 c0 30             	add    $0x30,%eax
     ae2:	88 45 e4             	mov    %al,-0x1c(%ebp)
     ae5:	b8 50 00 00 00       	mov    $0x50,%eax
     aea:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
     af0:	80 cc 0f             	or     $0xf,%ah
     af3:	83 c1 02             	add    $0x2,%ecx
     af6:	83 c2 01             	add    $0x1,%edx
     af9:	66 89 41 fe          	mov    %ax,-0x2(%ecx)
     afd:	66 0f be 42 ff       	movsbw -0x1(%edx),%ax
     b02:	84 c0                	test   %al,%al
     b04:	75 ea                	jne    0xaf0
     b06:	83 c4 3c             	add    $0x3c,%esp
     b09:	5b                   	pop    %ebx
     b0a:	5e                   	pop    %esi
     b0b:	5f                   	pop    %edi
     b0c:	5d                   	pop    %ebp
     b0d:	c3                   	ret
     b0e:	66 90                	xchg   %ax,%ax
     b10:	ba 30 00 00 00       	mov    $0x30,%edx
     b15:	b8 30 00 00 00       	mov    $0x30,%eax
     b1a:	66 89 55 e0          	mov    %dx,-0x20(%ebp)
     b1e:	e9 28 fe ff ff       	jmp    0x94b
     b23:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
     b27:	90                   	nop
     b28:	b9 30 00 00 00       	mov    $0x30,%ecx
     b2d:	b8 30 00 00 00       	mov    $0x30,%eax
     b32:	66 89 4d d6          	mov    %cx,-0x2a(%ebp)
     b36:	e9 97 fd ff ff       	jmp    0x8d2
     b3b:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
     b3f:	90                   	nop
     b40:	b8 30 00 00 00       	mov    $0x30,%eax
     b45:	66 89 45 d6          	mov    %ax,-0x2a(%ebp)
     b49:	b8 30 00 00 00       	mov    $0x30,%eax
     b4e:	e9 9a fe ff ff       	jmp    0x9ed
     b53:	31 db                	xor    %ebx,%ebx
     b55:	e9 5e ff ff ff       	jmp    0xab8
     b5a:	c7 45 d0 00 00 00 00 	movl   $0x0,-0x30(%ebp)
     b61:	e9 ed fe ff ff       	jmp    0xa53
     b66:	e9 c7 38 00 00       	jmp    0x4432
     b6b:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
     b6f:	90                   	nop
     b70:	55                   	push   %ebp
     b71:	ba 1f 85 eb 51       	mov    $0x51eb851f,%edx
     b76:	89 e5                	mov    %esp,%ebp
     b78:	57                   	push   %edi
     b79:	56                   	push   %esi
     b7a:	53                   	push   %ebx
     b7b:	83 ec 28             	sub    $0x28,%esp
     b7e:	8b 3d e8 6b 00 00    	mov    0x6be8,%edi
     b84:	a1 00 60 00 00       	mov    0x6000,%eax
     b89:	c7 45 ec 00 00 00 00 	movl   $0x0,-0x14(%ebp)
     b90:	c7 45 e4 a0 6c 00 00 	movl   $0x6ca0,-0x1c(%ebp)
     b97:	89 7d d4             	mov    %edi,-0x2c(%ebp)
     b9a:	8b 3d 48 6c 00 00    	mov    0x6c48,%edi
     ba0:	83 c0 01             	add    $0x1,%eax
     ba3:	a3 00 60 00 00       	mov    %eax,0x6000
     ba8:	89 45 e0             	mov    %eax,-0x20(%ebp)
     bab:	89 7d d0             	mov    %edi,-0x30(%ebp)
     bae:	89 c7                	mov    %eax,%edi
     bb0:	f7 e2                	mul    %edx
     bb2:	89 fe                	mov    %edi,%esi
     bb4:	c1 ea 05             	shr    $0x5,%edx
     bb7:	6b c2 64             	imul   $0x64,%edx,%eax
     bba:	29 c6                	sub    %eax,%esi
     bbc:	69 c7 29 5c 8f c2    	imul   $0xc28f5c29,%edi,%eax
     bc2:	8b 7d d4             	mov    -0x2c(%ebp),%edi
     bc5:	89 75 dc             	mov    %esi,-0x24(%ebp)
     bc8:	d1 c8                	ror    %eax
     bca:	3d 51 b8 1e 05       	cmp    $0x51eb851,%eax
     bcf:	8b 45 ec             	mov    -0x14(%ebp),%eax
     bd2:	0f 96 45 cf          	setbe  -0x31(%ebp)
     bd6:	39 f8                	cmp    %edi,%eax
     bd8:	0f 84 5a 01 00 00    	je     0xd38
     bde:	8b 7d d0             	mov    -0x30(%ebp),%edi
     be1:	ba 40 6c 00 00       	mov    $0x6c40,%edx
     be6:	39 f8                	cmp    %edi,%eax
     be8:	b8 00 00 00 00       	mov    $0x0,%eax
     bed:	0f 44 c2             	cmove  %edx,%eax
     bf0:	89 45 f0             	mov    %eax,-0x10(%ebp)
     bf3:	8b 45 ec             	mov    -0x14(%ebp),%eax
     bf6:	8b 4d e4             	mov    -0x1c(%ebp),%ecx
     bf9:	f7 d8                	neg    %eax
     bfb:	83 e0 0a             	and    $0xa,%eax
     bfe:	8d b0 a0 80 0b 00    	lea    0xb80a0(%eax),%esi
     c04:	31 c0                	xor    %eax,%eax
     c06:	89 f7                	mov    %esi,%edi
     c08:	89 c6                	mov    %eax,%esi
     c0a:	e9 d5 00 00 00       	jmp    0xce4
     c0f:	90                   	nop
     c10:	8b 45 e4             	mov    -0x1c(%ebp),%eax
     c13:	c7 45 e8 0a 00 00 00 	movl   $0xa,-0x18(%ebp)
     c1a:	80 78 78 01          	cmpb   $0x1,0x78(%eax)
     c1e:	74 09                	je     0xc29
     c20:	8b 45 f0             	mov    -0x10(%ebp),%eax
     c23:	8b 40 0c             	mov    0xc(%eax),%eax
     c26:	89 45 e8             	mov    %eax,-0x18(%ebp)
     c29:	8b 45 f0             	mov    -0x10(%ebp),%eax
     c2c:	8b 19                	mov    (%ecx),%ebx
     c2e:	8b 00                	mov    (%eax),%eax
     c30:	85 c0                	test   %eax,%eax
     c32:	0f 85 50 01 00 00    	jne    0xd88
     c38:	8d 42 02             	lea    0x2(%edx),%eax
     c3b:	89 c2                	mov    %eax,%edx
     c3d:	8b 45 e0             	mov    -0x20(%ebp),%eax
     c40:	89 55 d8             	mov    %edx,-0x28(%ebp)
     c43:	31 d2                	xor    %edx,%edx
     c45:	f7 75 d8             	divl   -0x28(%ebp)
     c48:	85 d2                	test   %edx,%edx
     c4a:	75 07                	jne    0xc53
     c4c:	8b 45 e8             	mov    -0x18(%ebp),%eax
     c4f:	01 c3                	add    %eax,%ebx
     c51:	89 19                	mov    %ebx,(%ecx)
     c53:	85 db                	test   %ebx,%ebx
     c55:	7e 05                	jle    0xc5c
     c57:	83 eb 05             	sub    $0x5,%ebx
     c5a:	89 19                	mov    %ebx,(%ecx)
     c5c:	8b 55 dc             	mov    -0x24(%ebp),%edx
     c5f:	85 d2                	test   %edx,%edx
     c61:	75 0e                	jne    0xc71
     c63:	8b 41 0c             	mov    0xc(%ecx),%eax
     c66:	83 f8 64             	cmp    $0x64,%eax
     c69:	7e 06                	jle    0xc71
     c6b:	83 e8 05             	sub    $0x5,%eax
     c6e:	89 41 0c             	mov    %eax,0xc(%ecx)
     c71:	8b 11                	mov    (%ecx),%edx
     c73:	8b 41 14             	mov    0x14(%ecx),%eax
     c76:	39 c2                	cmp    %eax,%edx
     c78:	0f 8c ca 00 00 00    	jl     0xd48
     c7e:	83 41 04 01          	addl   $0x1,0x4(%ecx)
     c82:	c7 01 00 00 00 00    	movl   $0x0,(%ecx)
     c88:	c7 41 10 0a 00 00 00 	movl   $0xa,0x10(%ecx)
     c8f:	3d 87 13 00 00       	cmp    $0x1387,%eax
     c94:	7f 08                	jg     0xc9e
     c96:	05 c8 00 00 00       	add    $0xc8,%eax
     c9b:	89 41 14             	mov    %eax,0x14(%ecx)
     c9e:	b8 cd cc cc cc       	mov    $0xcccccccd,%eax
     ca3:	f7 e6                	mul    %esi
     ca5:	8b 41 0c             	mov    0xc(%ecx),%eax
     ca8:	89 d3                	mov    %edx,%ebx
     caa:	83 e2 fc             	and    $0xfffffffc,%edx
     cad:	c1 eb 02             	shr    $0x2,%ebx
     cb0:	01 da                	add    %ebx,%edx
     cb2:	89 f3                	mov    %esi,%ebx
     cb4:	29 d3                	sub    %edx,%ebx
     cb6:	8b 55 ec             	mov    -0x14(%ebp),%edx
     cb9:	8d 1c 5b             	lea    (%ebx,%ebx,2),%ebx
     cbc:	c1 e2 07             	shl    $0x7,%edx
     cbf:	01 84 da a0 6c 00 00 	add    %eax,0x6ca0(%edx,%ebx,8)
     cc6:	3d 1f 03 00 00       	cmp    $0x31f,%eax
     ccb:	0f 8e cf 00 00 00    	jle    0xda0
     cd1:	b8 21 1c 00 00       	mov    $0x1c21,%eax
     cd6:	66 89 07             	mov    %ax,(%edi)
     cd9:	83 c1 18             	add    $0x18,%ecx
     cdc:	83 c7 02             	add    $0x2,%edi
     cdf:	83 fe 05             	cmp    $0x5,%esi
     ce2:	74 30                	je     0xd14
     ce4:	8b 41 10             	mov    0x10(%ecx),%eax
     ce7:	89 f2                	mov    %esi,%edx
     ce9:	83 c6 01             	add    $0x1,%esi
     cec:	85 c0                	test   %eax,%eax
     cee:	0f 8e 1c ff ff ff    	jle    0xc10
     cf4:	83 e8 01             	sub    $0x1,%eax
     cf7:	c7 01 00 00 00 00    	movl   $0x0,(%ecx)
     cfd:	83 c7 02             	add    $0x2,%edi
     d00:	83 c1 18             	add    $0x18,%ecx
     d03:	89 41 f8             	mov    %eax,-0x8(%ecx)
     d06:	b8 23 18 00 00       	mov    $0x1823,%eax
     d0b:	66 89 47 fe          	mov    %ax,-0x2(%edi)
     d0f:	83 fe 05             	cmp    $0x5,%esi
     d12:	75 d0                	jne    0xce4
     d14:	8b 45 ec             	mov    -0x14(%ebp),%eax
     d17:	83 6d e4 80          	subl   $0xffffff80,-0x1c(%ebp)
     d1b:	85 c0                	test   %eax,%eax
     d1d:	0f 85 ad 00 00 00    	jne    0xdd0
     d23:	c7 45 ec 01 00 00 00 	movl   $0x1,-0x14(%ebp)
     d2a:	8b 7d d4             	mov    -0x2c(%ebp),%edi
     d2d:	8b 45 ec             	mov    -0x14(%ebp),%eax
     d30:	39 f8                	cmp    %edi,%eax
     d32:	0f 85 a6 fe ff ff    	jne    0xbde
     d38:	c7 45 f0 e0 6b 00 00 	movl   $0x6be0,-0x10(%ebp)
     d3f:	e9 af fe ff ff       	jmp    0xbf3
     d44:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
     d48:	3d e8 03 00 00       	cmp    $0x3e8,%eax
     d4d:	7e 0c                	jle    0xd5b
     d4f:	80 7d cf 00          	cmpb   $0x0,-0x31(%ebp)
     d53:	74 06                	je     0xd5b
     d55:	83 e8 32             	sub    $0x32,%eax
     d58:	89 41 14             	mov    %eax,0x14(%ecx)
     d5b:	b8 5e 17 00 00       	mov    $0x175e,%eax
     d60:	81 fa bc 02 00 00    	cmp    $0x2bc,%edx
     d66:	0f 8f 6a ff ff ff    	jg     0xcd6
     d6c:	81 fa 2c 01 00 00    	cmp    $0x12c,%edx
     d72:	b8 2e 17 00 00       	mov    $0x172e,%eax
     d77:	ba 20 1f 00 00       	mov    $0x1f20,%edx
     d7c:	0f 4e c2             	cmovle %edx,%eax
     d7f:	e9 52 ff ff ff       	jmp    0xcd6
     d84:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
     d88:	f6 45 e0 03          	testb  $0x3,-0x20(%ebp)
     d8c:	0f 85 c1 fe ff ff    	jne    0xc53
     d92:	83 c3 19             	add    $0x19,%ebx
     d95:	89 19                	mov    %ebx,(%ecx)
     d97:	e9 b7 fe ff ff       	jmp    0xc53
     d9c:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
     da0:	83 c0 0a             	add    $0xa,%eax
     da3:	89 41 0c             	mov    %eax,0xc(%ecx)
     da6:	3d f4 01 00 00       	cmp    $0x1f4,%eax
     dab:	0f 8f 20 ff ff ff    	jg     0xcd1
     db1:	3d 2c 01 00 00       	cmp    $0x12c,%eax
     db6:	ba 21 1a 00 00       	mov    $0x1a21,%edx
     dbb:	b8 21 1e 00 00       	mov    $0x1e21,%eax
     dc0:	0f 4e c2             	cmovle %edx,%eax
     dc3:	e9 0e ff ff ff       	jmp    0xcd6
     dc8:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
     dcf:	90                   	nop
     dd0:	8b 45 e0             	mov    -0x20(%ebp),%eax
     dd3:	ba cd cc cc cc       	mov    $0xcccccccd,%edx
     dd8:	f7 e2                	mul    %edx
     dda:	c1 ea 04             	shr    $0x4,%edx
     ddd:	8d 04 92             	lea    (%edx,%edx,4),%eax
     de0:	8b 55 e0             	mov    -0x20(%ebp),%edx
     de3:	c1 e0 02             	shl    $0x2,%eax
     de6:	29 c2                	sub    %eax,%edx
     de8:	83 fa 0a             	cmp    $0xa,%edx
     deb:	ba 20 1f 00 00       	mov    $0x1f20,%edx
     df0:	19 c0                	sbb    %eax,%eax
     df2:	83 e0 0a             	and    $0xa,%eax
     df5:	66 05 20 1f          	add    $0x1f20,%ax
     df9:	66 a3 9e 80 0b 00    	mov    %ax,0xb809e
     dff:	a1 a4 6c 00 00       	mov    0x6ca4,%eax
     e04:	03 05 bc 6c 00 00    	add    0x6cbc,%eax
     e0a:	03 05 d4 6c 00 00    	add    0x6cd4,%eax
     e10:	03 05 ec 6c 00 00    	add    0x6cec,%eax
     e16:	03 05 04 6d 00 00    	add    0x6d04,%eax
     e1c:	03 05 24 6d 00 00    	add    0x6d24,%eax
     e22:	03 05 3c 6d 00 00    	add    0x6d3c,%eax
     e28:	03 05 54 6d 00 00    	add    0x6d54,%eax
     e2e:	03 05 6c 6d 00 00    	add    0x6d6c,%eax
     e34:	03 05 84 6d 00 00    	add    0x6d84,%eax
     e3a:	3d 88 13 00 00       	cmp    $0x1388,%eax
     e3f:	7e 50                	jle    0xe91
     e41:	31 c0                	xor    %eax,%eax
     e43:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
     e47:	90                   	nop
     e48:	8b 90 ac 6c 00 00    	mov    0x6cac(%eax),%edx
     e4e:	83 fa 64             	cmp    $0x64,%edx
     e51:	7e 09                	jle    0xe5c
     e53:	83 ea 0a             	sub    $0xa,%edx
     e56:	89 90 ac 6c 00 00    	mov    %edx,0x6cac(%eax)
     e5c:	83 c0 18             	add    $0x18,%eax
     e5f:	83 f8 78             	cmp    $0x78,%eax
     e62:	75 e4                	jne    0xe48
     e64:	31 c0                	xor    %eax,%eax
     e66:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
     e6d:	8d 76 00             	lea    0x0(%esi),%esi
     e70:	8b 90 2c 6d 00 00    	mov    0x6d2c(%eax),%edx
     e76:	83 fa 64             	cmp    $0x64,%edx
     e79:	7e 09                	jle    0xe84
     e7b:	83 ea 0a             	sub    $0xa,%edx
     e7e:	89 90 2c 6d 00 00    	mov    %edx,0x6d2c(%eax)
     e84:	83 c0 18             	add    $0x18,%eax
     e87:	83 f8 78             	cmp    $0x78,%eax
     e8a:	75 e4                	jne    0xe70
     e8c:	ba 21 4c 00 00       	mov    $0x4c21,%edx
     e91:	69 45 e0 cd cc cc cc 	imul   $0xcccccccd,-0x20(%ebp),%eax
     e98:	66 89 15 9c 80 0b 00 	mov    %dx,0xb809c
     e9f:	d1 c8                	ror    %eax
     ea1:	3d 99 99 99 19       	cmp    $0x19999999,%eax
     ea6:	76 08                	jbe    0xeb0
     ea8:	83 c4 28             	add    $0x28,%esp
     eab:	5b                   	pop    %ebx
     eac:	5e                   	pop    %esi
     ead:	5f                   	pop    %edi
     eae:	5d                   	pop    %ebp
     eaf:	c3                   	ret
     eb0:	83 c4 28             	add    $0x28,%esp
     eb3:	5b                   	pop    %ebx
     eb4:	5e                   	pop    %esi
     eb5:	5f                   	pop    %edi
     eb6:	5d                   	pop    %ebp
     eb7:	e9 84 f8 ff ff       	jmp    0x740
     ebc:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
     ec0:	55                   	push   %ebp
     ec1:	89 e5                	mov    %esp,%ebp
     ec3:	57                   	push   %edi
     ec4:	56                   	push   %esi
     ec5:	83 ec 60             	sub    $0x60,%esp
     ec8:	8b 35 e8 6b 00 00    	mov    0x6be8,%esi
     ece:	a1 48 6c 00 00       	mov    0x6c48,%eax
     ed3:	85 f6                	test   %esi,%esi
     ed5:	0f 84 c5 00 00 00    	je     0xfa0
     edb:	ba 01 00 00 00       	mov    $0x1,%edx
     ee0:	85 c0                	test   %eax,%eax
     ee2:	0f 84 ba 00 00 00    	je     0xfa2
     ee8:	83 fe 01             	cmp    $0x1,%esi
     eeb:	0f 84 ef 00 00 00    	je     0xfe0
     ef1:	83 f8 01             	cmp    $0x1,%eax
     ef4:	0f 84 e8 00 00 00    	je     0xfe2
     efa:	b8 e0 6b 00 00       	mov    $0x6be0,%eax
     eff:	8d 7d 98             	lea    -0x68(%ebp),%edi
     f02:	b9 18 00 00 00       	mov    $0x18,%ecx
     f07:	89 c6                	mov    %eax,%esi
     f09:	31 c0                	xor    %eax,%eax
     f0b:	f3 a5                	rep movsl %ds:(%esi),%es:(%edi)
     f0d:	bf e0 6b 00 00       	mov    $0x6be0,%edi
     f12:	b9 18 00 00 00       	mov    $0x18,%ecx
     f17:	f3 a5                	rep movsl %ds:(%esi),%es:(%edi)
     f19:	8d 75 98             	lea    -0x68(%ebp),%esi
     f1c:	b9 18 00 00 00       	mov    $0x18,%ecx
     f21:	c7 05 e8 6b 00 00 00 	movl   $0x0,0x6be8
     f28:	00 00 00 
     f2b:	f3 a5                	rep movsl %ds:(%esi),%es:(%edi)
     f2d:	b9 a0 6c 00 00       	mov    $0x6ca0,%ecx
     f32:	c7 05 48 6c 00 00 01 	movl   $0x1,0x6c48
     f39:	00 00 00 
     f3c:	89 ca                	mov    %ecx,%edx
     f3e:	8b 34 85 f8 6b 00 00 	mov    0x6bf8(,%eax,4),%esi
     f45:	83 c2 18             	add    $0x18,%edx
     f48:	89 72 e8             	mov    %esi,-0x18(%edx)
     f4b:	8b 34 85 0c 6c 00 00 	mov    0x6c0c(,%eax,4),%esi
     f52:	89 72 f4             	mov    %esi,-0xc(%edx)
     f55:	8b 34 85 20 6c 00 00 	mov    0x6c20(,%eax,4),%esi
     f5c:	83 c0 01             	add    $0x1,%eax
     f5f:	89 72 fc             	mov    %esi,-0x4(%edx)
     f62:	83 f8 05             	cmp    $0x5,%eax
     f65:	75 d7                	jne    0xf3e
     f67:	31 c0                	xor    %eax,%eax
     f69:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
     f70:	8b 14 85 58 6c 00 00 	mov    0x6c58(,%eax,4),%edx
     f77:	83 c1 18             	add    $0x18,%ecx
     f7a:	89 51 68             	mov    %edx,0x68(%ecx)
     f7d:	8b 14 85 6c 6c 00 00 	mov    0x6c6c(,%eax,4),%edx
     f84:	89 51 74             	mov    %edx,0x74(%ecx)
     f87:	8b 14 85 80 6c 00 00 	mov    0x6c80(,%eax,4),%edx
     f8e:	83 c0 01             	add    $0x1,%eax
     f91:	89 51 7c             	mov    %edx,0x7c(%ecx)
     f94:	83 f8 05             	cmp    $0x5,%eax
     f97:	75 d7                	jne    0xf70
     f99:	83 c4 60             	add    $0x60,%esp
     f9c:	5e                   	pop    %esi
     f9d:	5f                   	pop    %edi
     f9e:	5d                   	pop    %ebp
     f9f:	c3                   	ret
     fa0:	31 d2                	xor    %edx,%edx
     fa2:	f7 da                	neg    %edx
     fa4:	b9 a0 6c 00 00       	mov    $0x6ca0,%ecx
     fa9:	83 e2 60             	and    $0x60,%edx
     fac:	81 c2 e0 6b 00 00    	add    $0x6be0,%edx
     fb2:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
     fb8:	8b 39                	mov    (%ecx),%edi
     fba:	83 c1 18             	add    $0x18,%ecx
     fbd:	83 c2 04             	add    $0x4,%edx
     fc0:	89 7a 14             	mov    %edi,0x14(%edx)
     fc3:	8b 79 f4             	mov    -0xc(%ecx),%edi
     fc6:	89 7a 28             	mov    %edi,0x28(%edx)
     fc9:	8b 79 fc             	mov    -0x4(%ecx),%edi
     fcc:	89 7a 3c             	mov    %edi,0x3c(%edx)
     fcf:	81 f9 18 6d 00 00    	cmp    $0x6d18,%ecx
     fd5:	75 e1                	jne    0xfb8
     fd7:	e9 0c ff ff ff       	jmp    0xee8
     fdc:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
     fe0:	31 c0                	xor    %eax,%eax
     fe2:	f7 d8                	neg    %eax
     fe4:	83 e0 60             	and    $0x60,%eax
     fe7:	8d 90 e0 6b 00 00    	lea    0x6be0(%eax),%edx
     fed:	31 c0                	xor    %eax,%eax
     fef:	90                   	nop
     ff0:	8b 88 20 6d 00 00    	mov    0x6d20(%eax),%ecx
     ff6:	83 c0 18             	add    $0x18,%eax
     ff9:	83 c2 04             	add    $0x4,%edx
     ffc:	89 4a 14             	mov    %ecx,0x14(%edx)
     fff:	8b 88 14 6d 00 00    	mov    0x6d14(%eax),%ecx
    1005:	89 4a 28             	mov    %ecx,0x28(%edx)
    1008:	8b 88 1c 6d 00 00    	mov    0x6d1c(%eax),%ecx
    100e:	89 4a 3c             	mov    %ecx,0x3c(%edx)
    1011:	83 f8 78             	cmp    $0x78,%eax
    1014:	75 da                	jne    0xff0
    1016:	e9 df fe ff ff       	jmp    0xefa
    101b:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    101f:	90                   	nop
    1020:	55                   	push   %ebp
    1021:	89 e5                	mov    %esp,%ebp
    1023:	0f b6 45 08          	movzbl 0x8(%ebp),%eax
    1027:	5d                   	pop    %ebp
    1028:	0f b6 80 60 54 00 00 	movzbl 0x5460(%eax),%eax
    102f:	c3                   	ret
    1030:	55                   	push   %ebp
    1031:	89 e5                	mov    %esp,%ebp
    1033:	8b 55 0c             	mov    0xc(%ebp),%edx
    1036:	8b 45 08             	mov    0x8(%ebp),%eax
    1039:	83 fa 03             	cmp    $0x3,%edx
    103c:	7f 0a                	jg     0x1048
    103e:	5d                   	pop    %ebp
    103f:	e9 cc f3 ff ff       	jmp    0x410
    1044:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    1048:	5d                   	pop    %ebp
    1049:	c3                   	ret
    104a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    1050:	55                   	push   %ebp
    1051:	89 e5                	mov    %esp,%ebp
    1053:	57                   	push   %edi
    1054:	56                   	push   %esi
    1055:	53                   	push   %ebx
    1056:	83 ec 1c             	sub    $0x1c,%esp
    1059:	8b 5d 0c             	mov    0xc(%ebp),%ebx
    105c:	83 fb 03             	cmp    $0x3,%ebx
    105f:	0f 8f 0b 01 00 00    	jg     0x1170
    1065:	a1 2c 6e 00 00       	mov    0x6e2c,%eax
    106a:	8d 34 dd 00 00 00 00 	lea    0x0(,%ebx,8),%esi
    1071:	85 c0                	test   %eax,%eax
    1073:	0f 85 07 01 00 00    	jne    0x1180
    1079:	8d 0c 1e             	lea    (%esi,%ebx,1),%ecx
    107c:	83 c3 30             	add    $0x30,%ebx
    107f:	8d 04 cd c0 6a 00 00 	lea    0x6ac0(,%ecx,8),%eax
    1086:	80 cf 0c             	or     $0xc,%bh
    1089:	8b 50 08             	mov    0x8(%eax),%edx
    108c:	b8 4c 0c 00 00       	mov    $0xc4c,%eax
    1091:	66 89 1d ec 86 0b 00 	mov    %bx,0xb86ec
    1098:	bb 3a 0c 00 00       	mov    $0xc3a,%ebx
    109d:	66 a3 ea 86 0b 00    	mov    %ax,0xb86ea
    10a3:	8d 42 30             	lea    0x30(%edx),%eax
    10a6:	66 89 1d ee 86 0b 00 	mov    %bx,0xb86ee
    10ad:	80 cc 0c             	or     $0xc,%ah
    10b0:	66 a3 f0 86 0b 00    	mov    %ax,0xb86f0
    10b6:	85 d2                	test   %edx,%edx
    10b8:	0f 84 b2 00 00 00    	je     0x1170
    10be:	8b 45 08             	mov    0x8(%ebp),%eax
    10c1:	8d 34 09             	lea    (%ecx,%ecx,1),%esi
    10c4:	31 ff                	xor    %edi,%edi
    10c6:	89 75 e0             	mov    %esi,-0x20(%ebp)
    10c9:	8d 04 40             	lea    (%eax,%eax,2),%eax
    10cc:	c1 e0 05             	shl    $0x5,%eax
    10cf:	8b 90 e8 6b 00 00    	mov    0x6be8(%eax),%edx
    10d5:	05 e0 6b 00 00       	add    $0x6be0,%eax
    10da:	c1 e2 07             	shl    $0x7,%edx
    10dd:	81 c2 a0 6c 00 00    	add    $0x6ca0,%edx
    10e3:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    10e7:	90                   	nop
    10e8:	8b 75 e0             	mov    -0x20(%ebp),%esi
    10eb:	83 c2 18             	add    $0x18,%edx
    10ee:	83 c0 04             	add    $0x4,%eax
    10f1:	8d 0c 3e             	lea    (%esi,%edi,1),%ecx
    10f4:	83 c7 01             	add    $0x1,%edi
    10f7:	8b 1c 8d cc 6a 00 00 	mov    0x6acc(,%ecx,4),%ebx
    10fe:	8d 71 08             	lea    0x8(%ecx),%esi
    1101:	89 75 e4             	mov    %esi,-0x1c(%ebp)
    1104:	89 5a e8             	mov    %ebx,-0x18(%edx)
    1107:	8b 1c b5 c0 6a 00 00 	mov    0x6ac0(,%esi,4),%ebx
    110e:	89 5a f4             	mov    %ebx,-0xc(%edx)
    1111:	8d 59 0c             	lea    0xc(%ecx),%ebx
    1114:	8b 34 9d c4 6a 00 00 	mov    0x6ac4(,%ebx,4),%esi
    111b:	8b 0c 8d cc 6a 00 00 	mov    0x6acc(,%ecx,4),%ecx
    1122:	89 72 fc             	mov    %esi,-0x4(%edx)
    1125:	8b 75 e4             	mov    -0x1c(%ebp),%esi
    1128:	89 48 14             	mov    %ecx,0x14(%eax)
    112b:	8b 0c b5 c0 6a 00 00 	mov    0x6ac0(,%esi,4),%ecx
    1132:	89 48 28             	mov    %ecx,0x28(%eax)
    1135:	8b 0c 9d c4 6a 00 00 	mov    0x6ac4(,%ebx,4),%ecx
    113c:	89 48 3c             	mov    %ecx,0x3c(%eax)
    113f:	83 ff 05             	cmp    $0x5,%edi
    1142:	75 a4                	jne    0x10e8
    1144:	a1 2c 6e 00 00       	mov    0x6e2c,%eax
    1149:	83 f8 01             	cmp    $0x1,%eax
    114c:	19 c0                	sbb    %eax,%eax
    114e:	83 e0 09             	and    $0x9,%eax
    1151:	66 05 44 1f          	add    $0x1f44,%ax
    1155:	66 a3 9e 80 0b 00    	mov    %ax,0xb809e
    115b:	8d 65 f4             	lea    -0xc(%ebp),%esp
    115e:	b8 01 00 00 00       	mov    $0x1,%eax
    1163:	5b                   	pop    %ebx
    1164:	5e                   	pop    %esi
    1165:	5f                   	pop    %edi
    1166:	5d                   	pop    %ebp
    1167:	c3                   	ret
    1168:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    116f:	90                   	nop
    1170:	8d 65 f4             	lea    -0xc(%ebp),%esp
    1173:	31 c0                	xor    %eax,%eax
    1175:	5b                   	pop    %ebx
    1176:	5e                   	pop    %esi
    1177:	5f                   	pop    %edi
    1178:	5d                   	pop    %ebp
    1179:	c3                   	ret
    117a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    1180:	8d 04 1e             	lea    (%esi,%ebx,1),%eax
    1183:	83 ec 08             	sub    $0x8,%esp
    1186:	8d 04 c5 c0 6a 00 00 	lea    0x6ac0(,%eax,8),%eax
    118d:	50                   	push   %eax
    118e:	8d 43 42             	lea    0x42(%ebx),%eax
    1191:	50                   	push   %eax
    1192:	e8 a9 20 00 00       	call   0x3240
    1197:	83 c4 10             	add    $0x10,%esp
    119a:	e9 da fe ff ff       	jmp    0x1079
    119f:	90                   	nop
    11a0:	b9 a0 8a 0b 00       	mov    $0xb8aa0,%ecx
    11a5:	8d 76 00             	lea    0x0(%esi),%esi
    11a8:	8d 81 60 ff ff ff    	lea    -0xa0(%ecx),%eax
    11ae:	66 90                	xchg   %ax,%ax
    11b0:	0f b7 10             	movzwl (%eax),%edx
    11b3:	83 c0 02             	add    $0x2,%eax
    11b6:	66 89 90 5e ff ff ff 	mov    %dx,-0xa2(%eax)
    11bd:	39 c8                	cmp    %ecx,%eax
    11bf:	75 ef                	jne    0x11b0
    11c1:	8d 88 a0 00 00 00    	lea    0xa0(%eax),%ecx
    11c7:	3d a0 8f 0b 00       	cmp    $0xb8fa0,%eax
    11cc:	75 da                	jne    0x11a8
    11ce:	b8 00 8f 0b 00       	mov    $0xb8f00,%eax
    11d3:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    11d7:	90                   	nop
    11d8:	ba 20 07 00 00       	mov    $0x720,%edx
    11dd:	b9 20 07 00 00       	mov    $0x720,%ecx
    11e2:	83 c0 04             	add    $0x4,%eax
    11e5:	66 89 50 fc          	mov    %dx,-0x4(%eax)
    11e9:	66 89 48 fe          	mov    %cx,-0x2(%eax)
    11ed:	3d a0 8f 0b 00       	cmp    $0xb8fa0,%eax
    11f2:	75 e4                	jne    0x11d8
    11f4:	c7 05 34 59 00 00 18 	movl   $0x18,0x5934
    11fb:	00 00 00 
    11fe:	c3                   	ret
    11ff:	90                   	nop
    1200:	55                   	push   %ebp
    1201:	b8 30 78 00 00       	mov    $0x7830,%eax
    1206:	89 e5                	mov    %esp,%ebp
    1208:	56                   	push   %esi
    1209:	8b 75 0c             	mov    0xc(%ebp),%esi
    120c:	8b 55 08             	mov    0x8(%ebp),%edx
    120f:	53                   	push   %ebx
    1210:	66 89 06             	mov    %ax,(%esi)
    1213:	8d 5e 01             	lea    0x1(%esi),%ebx
    1216:	8d 46 09             	lea    0x9(%esi),%eax
    1219:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    1220:	89 d1                	mov    %edx,%ecx
    1222:	83 e8 01             	sub    $0x1,%eax
    1225:	c1 ea 04             	shr    $0x4,%edx
    1228:	83 e1 0f             	and    $0xf,%ecx
    122b:	0f b6 89 0e 55 00 00 	movzbl 0x550e(%ecx),%ecx
    1232:	88 48 01             	mov    %cl,0x1(%eax)
    1235:	39 d8                	cmp    %ebx,%eax
    1237:	75 e7                	jne    0x1220
    1239:	c6 46 0a 00          	movb   $0x0,0xa(%esi)
    123d:	5b                   	pop    %ebx
    123e:	5e                   	pop    %esi
    123f:	5d                   	pop    %ebp
    1240:	c3                   	ret
    1241:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    1248:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    124f:	90                   	nop
    1250:	55                   	push   %ebp
    1251:	31 c9                	xor    %ecx,%ecx
    1253:	89 e5                	mov    %esp,%ebp
    1255:	53                   	push   %ebx
    1256:	8b 55 08             	mov    0x8(%ebp),%edx
    1259:	0f be 02             	movsbl (%edx),%eax
    125c:	3c 30                	cmp    $0x30,%al
    125e:	75 16                	jne    0x1276
    1260:	0f b6 42 01          	movzbl 0x1(%edx),%eax
    1264:	83 e0 df             	and    $0xffffffdf,%eax
    1267:	3c 58                	cmp    $0x58,%al
    1269:	0f 94 c0             	sete   %al
    126c:	0f b6 c0             	movzbl %al,%eax
    126f:	8d 0c 00             	lea    (%eax,%eax,1),%ecx
    1272:	0f be 04 42          	movsbl (%edx,%eax,2),%eax
    1276:	8d 5c 0a 01          	lea    0x1(%edx,%ecx,1),%ebx
    127a:	31 d2                	xor    %edx,%edx
    127c:	a8 df                	test   $0xdf,%al
    127e:	75 1a                	jne    0x129a
    1280:	eb 42                	jmp    0x12c4
    1282:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    1288:	8d 48 a9             	lea    -0x57(%eax),%ecx
    128b:	0f be 03             	movsbl (%ebx),%eax
    128e:	c1 e2 04             	shl    $0x4,%edx
    1291:	83 c3 01             	add    $0x1,%ebx
    1294:	09 ca                	or     %ecx,%edx
    1296:	a8 df                	test   $0xdf,%al
    1298:	74 2a                	je     0x12c4
    129a:	8d 48 d0             	lea    -0x30(%eax),%ecx
    129d:	83 f9 09             	cmp    $0x9,%ecx
    12a0:	76 e9                	jbe    0x128b
    12a2:	8d 48 9f             	lea    -0x61(%eax),%ecx
    12a5:	83 f9 05             	cmp    $0x5,%ecx
    12a8:	76 de                	jbe    0x1288
    12aa:	8d 48 bf             	lea    -0x41(%eax),%ecx
    12ad:	83 f9 05             	cmp    $0x5,%ecx
    12b0:	77 12                	ja     0x12c4
    12b2:	8d 48 c9             	lea    -0x37(%eax),%ecx
    12b5:	0f be 03             	movsbl (%ebx),%eax
    12b8:	c1 e2 04             	shl    $0x4,%edx
    12bb:	83 c3 01             	add    $0x1,%ebx
    12be:	09 ca                	or     %ecx,%edx
    12c0:	a8 df                	test   $0xdf,%al
    12c2:	75 d6                	jne    0x129a
    12c4:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    12c7:	89 d0                	mov    %edx,%eax
    12c9:	c9                   	leave
    12ca:	c3                   	ret
    12cb:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    12cf:	90                   	nop
    12d0:	55                   	push   %ebp
    12d1:	89 e5                	mov    %esp,%ebp
    12d3:	53                   	push   %ebx
    12d4:	83 ec 10             	sub    $0x10,%esp
    12d7:	68 22 11 00 00       	push   $0x1122
    12dc:	6a 64                	push   $0x64
    12de:	68 20 03 00 00       	push   $0x320
    12e3:	6a 00                	push   $0x0
    12e5:	6a 00                	push   $0x0
    12e7:	e8 a4 28 00 00       	call   0x3b90
    12ec:	83 c4 20             	add    $0x20,%esp
    12ef:	68 ff ff 00 00       	push   $0xffff
    12f4:	68 20 03 00 00       	push   $0x320
    12f9:	6a 00                	push   $0x0
    12fb:	6a 64                	push   $0x64
    12fd:	e8 1e 29 00 00       	call   0x3c20
    1302:	68 66 66 00 00       	push   $0x6666
    1307:	68 20 03 00 00       	push   $0x320
    130c:	6a 00                	push   $0x0
    130e:	6a 65                	push   $0x65
    1310:	e8 0b 29 00 00       	call   0x3c20
    1315:	83 c4 18             	add    $0x18,%esp
    1318:	c7 05 60 b0 00 00 04 	movl   $0x4,0xb060
    131f:	00 00 00 
    1322:	68 ff ff 00 00       	push   $0xffff
    1327:	68 1f 55 00 00       	push   $0x551f
    132c:	c7 05 60 59 00 00 08 	movl   $0x8,0x5960
    1333:	00 00 00 
    1336:	e8 b5 2a 00 00       	call   0x3df0
    133b:	58                   	pop    %eax
    133c:	5a                   	pop    %edx
    133d:	68 66 66 44 00       	push   $0x446666
    1342:	68 2e 55 00 00       	push   $0x552e
    1347:	e8 a4 2a 00 00       	call   0x3df0
    134c:	59                   	pop    %ecx
    134d:	5b                   	pop    %ebx
    134e:	68 00 ff ff 00       	push   $0xffff00
    1353:	68 31 55 00 00       	push   $0x5531
    1358:	e8 93 2a 00 00       	call   0x3df0
    135d:	58                   	pop    %eax
    135e:	5a                   	pop    %edx
    135f:	68 ff ff ff 00       	push   $0xffffff
    1364:	ff 35 80 fc 1d 00    	push   0x1dfc80
    136a:	e8 b1 2b 00 00       	call   0x3f20
    136f:	59                   	pop    %ecx
    1370:	5b                   	pop    %ebx
    1371:	68 00 ff ff 00       	push   $0xffff00
    1376:	68 37 55 00 00       	push   $0x5537
    137b:	e8 70 2a 00 00       	call   0x3df0
    1380:	a1 80 fc 1d 00       	mov    0x1dfc80,%eax
    1385:	83 c4 10             	add    $0x10,%esp
    1388:	85 c0                	test   %eax,%eax
    138a:	7e 76                	jle    0x1402
    138c:	31 db                	xor    %ebx,%ebx
    138e:	eb 0b                	jmp    0x139b
    1390:	83 c3 01             	add    $0x1,%ebx
    1393:	39 1d 80 fc 1d 00    	cmp    %ebx,0x1dfc80
    1399:	7e 67                	jle    0x1402
    139b:	80 3c dd a4 fc 1d 00 	cmpb   $0x1,0x1dfca4(,%ebx,8)
    13a2:	01 
    13a3:	75 eb                	jne    0x1390
    13a5:	0f b6 04 dd a5 fc 1d 	movzbl 0x1dfca5(,%ebx,8),%eax
    13ac:	00 
    13ad:	3c 08                	cmp    $0x8,%al
    13af:	0f 84 63 01 00 00    	je     0x1518
    13b5:	3c 06                	cmp    $0x6,%al
    13b7:	75 d7                	jne    0x1390
    13b9:	83 ec 08             	sub    $0x8,%esp
    13bc:	68 cc ff 00 00       	push   $0xffcc
    13c1:	68 49 55 00 00       	push   $0x5549
    13c6:	e8 25 2a 00 00       	call   0x3df0
    13cb:	0f b6 04 dd a6 fc 1d 	movzbl 0x1dfca6(,%ebx,8),%eax
    13d2:	00 
    13d3:	83 c4 0c             	add    $0xc,%esp
    13d6:	83 c3 01             	add    $0x1,%ebx
    13d9:	68 cc ff 00 00       	push   $0xffcc
    13de:	6a 02                	push   $0x2
    13e0:	50                   	push   %eax
    13e1:	e8 ca 2a 00 00       	call   0x3eb0
    13e6:	58                   	pop    %eax
    13e7:	5a                   	pop    %edx
    13e8:	68 cc ff 00 00       	push   $0xffcc
    13ed:	68 46 55 00 00       	push   $0x5546
    13f2:	e8 f9 29 00 00       	call   0x3df0
    13f7:	83 c4 10             	add    $0x10,%esp
    13fa:	39 1d 80 fc 1d 00    	cmp    %ebx,0x1dfc80
    1400:	7f 99                	jg     0x139b
    1402:	c7 05 60 b0 00 00 04 	movl   $0x4,0xb060
    1409:	00 00 00 
    140c:	83 ec 08             	sub    $0x8,%esp
    140f:	68 aa aa aa 00       	push   $0xaaaaaa
    1414:	68 51 55 00 00       	push   $0x5551
    1419:	c7 05 60 59 00 00 16 	movl   $0x16,0x5960
    1420:	00 00 00 
    1423:	e8 c8 29 00 00       	call   0x3df0
    1428:	e8 03 23 00 00       	call   0x3730
    142d:	89 c3                	mov    %eax,%ebx
    142f:	58                   	pop    %eax
    1430:	5a                   	pop    %edx
    1431:	68 88 ff 88 00       	push   $0x88ff88
    1436:	53                   	push   %ebx
    1437:	c1 e3 02             	shl    $0x2,%ebx
    143a:	e8 e1 2a 00 00       	call   0x3f20
    143f:	59                   	pop    %ecx
    1440:	58                   	pop    %eax
    1441:	68 aa aa aa 00       	push   $0xaaaaaa
    1446:	68 5c 55 00 00       	push   $0x555c
    144b:	e8 a0 29 00 00       	call   0x3df0
    1450:	58                   	pop    %eax
    1451:	5a                   	pop    %edx
    1452:	68 88 ff 88 00       	push   $0x88ff88
    1457:	53                   	push   %ebx
    1458:	e8 c3 2a 00 00       	call   0x3f20
    145d:	59                   	pop    %ecx
    145e:	5b                   	pop    %ebx
    145f:	68 aa aa aa 00       	push   $0xaaaaaa
    1464:	68 65 55 00 00       	push   $0x5565
    1469:	e8 82 29 00 00       	call   0x3df0
    146e:	58                   	pop    %eax
    146f:	5a                   	pop    %edx
    1470:	68 aa aa aa 00       	push   $0xaaaaaa
    1475:	68 6a 55 00 00       	push   $0x556a
    147a:	c7 05 60 b0 00 00 04 	movl   $0x4,0xb060
    1481:	00 00 00 
    1484:	c7 05 60 59 00 00 24 	movl   $0x24,0x5960
    148b:	00 00 00 
    148e:	e8 5d 29 00 00       	call   0x3df0
    1493:	59                   	pop    %ecx
    1494:	5b                   	pop    %ebx
    1495:	68 00 88 ff 00       	push   $0xff8800
    149a:	6a 10                	push   $0x10
    149c:	e8 7f 2a 00 00       	call   0x3f20
    14a1:	58                   	pop    %eax
    14a2:	5a                   	pop    %edx
    14a3:	68 aa aa aa 00       	push   $0xaaaaaa
    14a8:	68 74 55 00 00       	push   $0x5574
    14ad:	e8 3e 29 00 00       	call   0x3df0
    14b2:	59                   	pop    %ecx
    14b3:	5b                   	pop    %ebx
    14b4:	68 00 88 ff 00       	push   $0xff8800
    14b9:	ff 35 00 60 00 00    	push   0x6000
    14bf:	e8 5c 2a 00 00       	call   0x3f20
    14c4:	58                   	pop    %eax
    14c5:	5a                   	pop    %edx
    14c6:	68 aa aa aa 00       	push   $0xaaaaaa
    14cb:	68 7d 55 00 00       	push   $0x557d
    14d0:	e8 1b 29 00 00       	call   0x3df0
    14d5:	59                   	pop    %ecx
    14d6:	5b                   	pop    %ebx
    14d7:	68 00 55 ff 00       	push   $0xff5500
    14dc:	ff 35 38 6c 00 00    	push   0x6c38
    14e2:	e8 39 2a 00 00       	call   0x3f20
    14e7:	58                   	pop    %eax
    14e8:	5a                   	pop    %edx
    14e9:	68 aa aa aa 00       	push   $0xaaaaaa
    14ee:	68 88 55 00 00       	push   $0x5588
    14f3:	e8 f8 28 00 00       	call   0x3df0
    14f8:	59                   	pop    %ecx
    14f9:	5b                   	pop    %ebx
    14fa:	68 00 55 ff 00       	push   $0xff5500
    14ff:	ff 35 98 6c 00 00    	push   0x6c98
    1505:	e8 16 2a 00 00       	call   0x3f20
    150a:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    150d:	83 c4 10             	add    $0x10,%esp
    1510:	c9                   	leave
    1511:	c3                   	ret
    1512:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    1518:	83 ec 08             	sub    $0x8,%esp
    151b:	68 88 ff 00 00       	push   $0xff88
    1520:	68 3e 55 00 00       	push   $0x553e
    1525:	e8 c6 28 00 00       	call   0x3df0
    152a:	0f b6 04 dd a6 fc 1d 	movzbl 0x1dfca6(,%ebx,8),%eax
    1531:	00 
    1532:	83 c4 0c             	add    $0xc,%esp
    1535:	68 88 ff 00 00       	push   $0xff88
    153a:	6a 02                	push   $0x2
    153c:	50                   	push   %eax
    153d:	e8 6e 29 00 00       	call   0x3eb0
    1542:	59                   	pop    %ecx
    1543:	58                   	pop    %eax
    1544:	68 88 ff 00 00       	push   $0xff88
    1549:	68 46 55 00 00       	push   $0x5546
    154e:	e8 9d 28 00 00       	call   0x3df0
    1553:	83 c4 10             	add    $0x10,%esp
    1556:	e9 35 fe ff ff       	jmp    0x1390
    155b:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    155f:	90                   	nop
    1560:	55                   	push   %ebp
    1561:	89 e5                	mov    %esp,%ebp
    1563:	57                   	push   %edi
    1564:	56                   	push   %esi
    1565:	53                   	push   %ebx
    1566:	83 ec 3c             	sub    $0x3c,%esp
    1569:	68 55 44 33 00       	push   $0x334455
    156e:	68 20 03 00 00       	push   $0x320
    1573:	6a 00                	push   $0x0
    1575:	6a 6d                	push   $0x6d
    1577:	e8 a4 26 00 00       	call   0x3c20
    157c:	68 55 44 33 00       	push   $0x334455
    1581:	68 20 03 00 00       	push   $0x320
    1586:	6a 00                	push   $0x0
    1588:	68 2c 01 00 00       	push   $0x12c
    158d:	e8 8e 26 00 00       	call   0x3c20
    1592:	83 c4 18             	add    $0x18,%esp
    1595:	c7 05 60 b0 00 00 04 	movl   $0x4,0xb060
    159c:	00 00 00 
    159f:	c7 05 60 59 00 00 6e 	movl   $0x6e,0x5960
    15a6:	00 00 00 
    15a9:	68 aa 66 33 00       	push   $0x3366aa
    15ae:	68 93 55 00 00       	push   $0x5593
    15b3:	e8 38 28 00 00       	call   0x3df0
    15b8:	c7 45 d8 00 00 00 00 	movl   $0x0,-0x28(%ebp)
    15bf:	83 c4 10             	add    $0x10,%esp
    15c2:	c7 45 d0 00 00 00 00 	movl   $0x0,-0x30(%ebp)
    15c9:	c7 45 cc ff ff ff ff 	movl   $0xffffffff,-0x34(%ebp)
    15d0:	c7 45 c8 ff ff ff ff 	movl   $0xffffffff,-0x38(%ebp)
    15d7:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    15de:	66 90                	xchg   %ax,%ax
    15e0:	8b 7d d0             	mov    -0x30(%ebp),%edi
    15e3:	b9 e8 03 00 00       	mov    $0x3e8,%ecx
    15e8:	8b 75 c8             	mov    -0x38(%ebp),%esi
    15eb:	8b 1c bd a0 6d 00 00 	mov    0x6da0(,%edi,4),%ebx
    15f2:	39 cb                	cmp    %ecx,%ebx
    15f4:	0f 4e cb             	cmovle %ebx,%ecx
    15f7:	31 c0                	xor    %eax,%eax
    15f9:	85 c9                	test   %ecx,%ecx
    15fb:	0f 48 c8             	cmovs  %eax,%ecx
    15fe:	83 c7 01             	add    $0x1,%edi
    1601:	b8 d3 4d 62 10       	mov    $0x10624dd3,%eax
    1606:	89 7d d0             	mov    %edi,-0x30(%ebp)
    1609:	8b 7d d8             	mov    -0x28(%ebp),%edi
    160c:	69 c9 aa 00 00 00    	imul   $0xaa,%ecx,%ecx
    1612:	f7 e1                	mul    %ecx
    1614:	89 f8                	mov    %edi,%eax
    1616:	83 c7 2f             	add    $0x2f,%edi
    1619:	89 7d d8             	mov    %edi,-0x28(%ebp)
    161c:	89 7d c8             	mov    %edi,-0x38(%ebp)
    161f:	8b 7d cc             	mov    -0x34(%ebp),%edi
    1622:	c1 ea 06             	shr    $0x6,%edx
    1625:	89 7d d4             	mov    %edi,-0x2c(%ebp)
    1628:	bf 22 01 00 00       	mov    $0x122,%edi
    162d:	29 d7                	sub    %edx,%edi
    162f:	89 7d cc             	mov    %edi,-0x34(%ebp)
    1632:	81 fb 4c 01 00 00    	cmp    $0x14c,%ebx
    1638:	0f 8e 0a 01 00 00    	jle    0x1748
    163e:	81 fb 99 02 00 00    	cmp    $0x299,%ebx
    1644:	b9 00 dd ff 00       	mov    $0xffdd00,%ecx
    1649:	bb 00 33 ff 00       	mov    $0xff3300,%ebx
    164e:	0f 4f cb             	cmovg  %ebx,%ecx
    1651:	89 cf                	mov    %ecx,%edi
    1653:	83 c0 2d             	add    $0x2d,%eax
    1656:	89 55 dc             	mov    %edx,-0x24(%ebp)
    1659:	31 c9                	xor    %ecx,%ecx
    165b:	89 45 e4             	mov    %eax,-0x1c(%ebp)
    165e:	89 75 c4             	mov    %esi,-0x3c(%ebp)
    1661:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    1668:	89 4d e0             	mov    %ecx,-0x20(%ebp)
    166b:	bb 22 01 00 00       	mov    $0x122,%ebx
    1670:	31 f6                	xor    %esi,%esi
    1672:	29 cb                	sub    %ecx,%ebx
    1674:	8b 45 e4             	mov    -0x1c(%ebp),%eax
    1677:	83 ec 04             	sub    $0x4,%esp
    167a:	57                   	push   %edi
    167b:	8d 14 30             	lea    (%eax,%esi,1),%edx
    167e:	53                   	push   %ebx
    167f:	83 c6 01             	add    $0x1,%esi
    1682:	52                   	push   %edx
    1683:	e8 78 24 00 00       	call   0x3b00
    1688:	83 c4 10             	add    $0x10,%esp
    168b:	83 fe 04             	cmp    $0x4,%esi
    168e:	75 e4                	jne    0x1674
    1690:	8b 4d e0             	mov    -0x20(%ebp),%ecx
    1693:	83 c1 01             	add    $0x1,%ecx
    1696:	39 4d dc             	cmp    %ecx,-0x24(%ebp)
    1699:	7f cd                	jg     0x1668
    169b:	8b 75 c4             	mov    -0x3c(%ebp),%esi
    169e:	83 fe ff             	cmp    $0xffffffff,%esi
    16a1:	0f 85 c1 00 00 00    	jne    0x1768
    16a7:	83 7d d0 10          	cmpl   $0x10,-0x30(%ebp)
    16ab:	0f 85 2f ff ff ff    	jne    0x15e0
    16b1:	c7 45 dc 00 00 00 00 	movl   $0x0,-0x24(%ebp)
    16b8:	c7 45 e4 00 00 00 00 	movl   $0x0,-0x1c(%ebp)
    16bf:	eb 11                	jmp    0x16d2
    16c1:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    16c8:	83 45 dc 2f          	addl   $0x2f,-0x24(%ebp)
    16cc:	83 7d e4 10          	cmpl   $0x10,-0x1c(%ebp)
    16d0:	74 67                	je     0x1739
    16d2:	8b 4d e4             	mov    -0x1c(%ebp),%ecx
    16d5:	8b 04 8d a0 6d 00 00 	mov    0x6da0(,%ecx,4),%eax
    16dc:	83 c1 01             	add    $0x1,%ecx
    16df:	89 4d e4             	mov    %ecx,-0x1c(%ebp)
    16e2:	3d b5 03 00 00       	cmp    $0x3b5,%eax
    16e7:	7e df                	jle    0x16c8
    16e9:	8b 45 dc             	mov    -0x24(%ebp),%eax
    16ec:	be 05 00 00 00       	mov    $0x5,%esi
    16f1:	83 c0 32             	add    $0x32,%eax
    16f4:	89 45 e0             	mov    %eax,-0x20(%ebp)
    16f7:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    16fe:	66 90                	xchg   %ax,%ax
    1700:	8b 5d e0             	mov    -0x20(%ebp),%ebx
    1703:	bf 76 00 00 00       	mov    $0x76,%edi
    1708:	29 f3                	sub    %esi,%ebx
    170a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    1710:	83 ec 04             	sub    $0x4,%esp
    1713:	68 ff ff ff 00       	push   $0xffffff
    1718:	57                   	push   %edi
    1719:	83 c7 01             	add    $0x1,%edi
    171c:	53                   	push   %ebx
    171d:	e8 de 23 00 00       	call   0x3b00
    1722:	83 c4 10             	add    $0x10,%esp
    1725:	83 ff 7b             	cmp    $0x7b,%edi
    1728:	75 e6                	jne    0x1710
    172a:	83 ee 01             	sub    $0x1,%esi
    172d:	75 d1                	jne    0x1700
    172f:	83 45 dc 2f          	addl   $0x2f,-0x24(%ebp)
    1733:	83 7d e4 10          	cmpl   $0x10,-0x1c(%ebp)
    1737:	75 99                	jne    0x16d2
    1739:	8d 65 f4             	lea    -0xc(%ebp),%esp
    173c:	5b                   	pop    %ebx
    173d:	5e                   	pop    %esi
    173e:	5f                   	pop    %edi
    173f:	5d                   	pop    %ebp
    1740:	c3                   	ret
    1741:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    1748:	bf 44 cc 00 00       	mov    $0xcc44,%edi
    174d:	81 f9 e7 03 00 00    	cmp    $0x3e7,%ecx
    1753:	0f 8f fa fe ff ff    	jg     0x1653
    1759:	83 fe ff             	cmp    $0xffffffff,%esi
    175c:	0f 84 45 ff ff ff    	je     0x16a7
    1762:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    1768:	8b 55 d8             	mov    -0x28(%ebp),%edx
    176b:	31 c9                	xor    %ecx,%ecx
    176d:	89 d0                	mov    %edx,%eax
    176f:	29 f0                	sub    %esi,%eax
    1771:	85 c0                	test   %eax,%eax
    1773:	0f 9f c1             	setg   %cl
    1776:	8d 4c 09 ff          	lea    -0x1(%ecx,%ecx,1),%ecx
    177a:	39 f2                	cmp    %esi,%edx
    177c:	0f 84 25 ff ff ff    	je     0x16a7
    1782:	85 c0                	test   %eax,%eax
    1784:	ba 01 00 00 00       	mov    $0x1,%edx
    1789:	8b 7d cc             	mov    -0x34(%ebp),%edi
    178c:	89 4d e0             	mov    %ecx,-0x20(%ebp)
    178f:	0f 45 d0             	cmovne %eax,%edx
    1792:	8b 45 d4             	mov    -0x2c(%ebp),%eax
    1795:	31 db                	xor    %ebx,%ebx
    1797:	29 c7                	sub    %eax,%edi
    1799:	89 55 e4             	mov    %edx,-0x1c(%ebp)
    179c:	0f af f9             	imul   %ecx,%edi
    179f:	90                   	nop
    17a0:	89 d8                	mov    %ebx,%eax
    17a2:	8b 4d d4             	mov    -0x2c(%ebp),%ecx
    17a5:	83 ec 04             	sub    $0x4,%esp
    17a8:	01 fb                	add    %edi,%ebx
    17aa:	99                   	cltd
    17ab:	68 88 55 00 00       	push   $0x5588
    17b0:	f7 7d e4             	idivl  -0x1c(%ebp)
    17b3:	01 c8                	add    %ecx,%eax
    17b5:	50                   	push   %eax
    17b6:	56                   	push   %esi
    17b7:	e8 44 23 00 00       	call   0x3b00
    17bc:	8b 45 e0             	mov    -0x20(%ebp),%eax
    17bf:	83 c4 10             	add    $0x10,%esp
    17c2:	01 c6                	add    %eax,%esi
    17c4:	39 75 d8             	cmp    %esi,-0x28(%ebp)
    17c7:	75 d7                	jne    0x17a0
    17c9:	e9 d9 fe ff ff       	jmp    0x16a7
    17ce:	66 90                	xchg   %ax,%ax
    17d0:	55                   	push   %ebp
    17d1:	89 e5                	mov    %esp,%ebp
    17d3:	57                   	push   %edi
    17d4:	56                   	push   %esi
    17d5:	53                   	push   %ebx
    17d6:	83 ec 2c             	sub    $0x2c,%esp
    17d9:	a1 2c 6e 00 00       	mov    0x6e2c,%eax
    17de:	85 c0                	test   %eax,%eax
    17e0:	0f 85 29 02 00 00    	jne    0x1a0f
    17e6:	a1 34 59 00 00       	mov    0x5934,%eax
    17eb:	31 d2                	xor    %edx,%edx
    17ed:	8d 0c 80             	lea    (%eax,%eax,4),%ecx
    17f0:	b8 4e 00 00 00       	mov    $0x4e,%eax
    17f5:	c1 e1 05             	shl    $0x5,%ecx
    17f8:	81 c1 00 80 0b 00    	add    $0xb8000,%ecx
    17fe:	eb 05                	jmp    0x1805
    1800:	83 fa 50             	cmp    $0x50,%edx
    1803:	74 19                	je     0x181e
    1805:	80 cc 0b             	or     $0xb,%ah
    1808:	83 c2 01             	add    $0x1,%edx
    180b:	83 c1 02             	add    $0x2,%ecx
    180e:	66 89 41 fe          	mov    %ax,-0x2(%ecx)
    1812:	66 0f be 82 ab 55 00 	movsbw 0x55ab(%edx),%ax
    1819:	00 
    181a:	84 c0                	test   %al,%al
    181c:	75 e2                	jne    0x1800
    181e:	a1 34 59 00 00       	mov    0x5934,%eax
    1823:	83 f8 17             	cmp    $0x17,%eax
    1826:	0f 8f 50 02 00 00    	jg     0x1a7c
    182c:	a1 34 59 00 00       	mov    0x5934,%eax
    1831:	83 c0 01             	add    $0x1,%eax
    1834:	a3 34 59 00 00       	mov    %eax,0x5934
    1839:	a1 34 59 00 00       	mov    0x5934,%eax
    183e:	31 d2                	xor    %edx,%edx
    1840:	8d 0c 80             	lea    (%eax,%eax,4),%ecx
    1843:	b8 2d 00 00 00       	mov    $0x2d,%eax
    1848:	c1 e1 05             	shl    $0x5,%ecx
    184b:	81 c1 00 80 0b 00    	add    $0xb8000,%ecx
    1851:	eb 09                	jmp    0x185c
    1853:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    1857:	90                   	nop
    1858:	84 c0                	test   %al,%al
    185a:	74 1a                	je     0x1876
    185c:	80 cc 07             	or     $0x7,%ah
    185f:	83 c2 01             	add    $0x1,%edx
    1862:	83 c1 02             	add    $0x2,%ecx
    1865:	66 89 41 fe          	mov    %ax,-0x2(%ecx)
    1869:	66 0f be 82 c4 55 00 	movsbw 0x55c4(%edx),%ax
    1870:	00 
    1871:	83 fa 50             	cmp    $0x50,%edx
    1874:	75 e2                	jne    0x1858
    1876:	a1 34 59 00 00       	mov    0x5934,%eax
    187b:	83 f8 17             	cmp    $0x17,%eax
    187e:	0f 8f ee 01 00 00    	jg     0x1a72
    1884:	a1 34 59 00 00       	mov    0x5934,%eax
    1889:	83 c0 01             	add    $0x1,%eax
    188c:	a3 34 59 00 00       	mov    %eax,0x5934
    1891:	bb 60 68 00 00       	mov    $0x6860,%ebx
    1896:	be b8 6a 00 00       	mov    $0x6ab8,%esi
    189b:	31 d2                	xor    %edx,%edx
    189d:	eb 0c                	jmp    0x18ab
    189f:	90                   	nop
    18a0:	83 c3 18             	add    $0x18,%ebx
    18a3:	39 de                	cmp    %ebx,%esi
    18a5:	0f 84 07 01 00 00    	je     0x19b2
    18ab:	83 7b 14 01          	cmpl   $0x1,0x14(%ebx)
    18af:	75 ef                	jne    0x18a0
    18b1:	66 0f be 03          	movsbw (%ebx),%ax
    18b5:	8b 15 34 59 00 00    	mov    0x5934,%edx
    18bb:	84 c0                	test   %al,%al
    18bd:	74 2c                	je     0x18eb
    18bf:	8d 0c 92             	lea    (%edx,%edx,4),%ecx
    18c2:	31 d2                	xor    %edx,%edx
    18c4:	c1 e1 05             	shl    $0x5,%ecx
    18c7:	81 c1 00 80 0b 00    	add    $0xb8000,%ecx
    18cd:	eb 05                	jmp    0x18d4
    18cf:	90                   	nop
    18d0:	84 c0                	test   %al,%al
    18d2:	74 17                	je     0x18eb
    18d4:	80 cc 0f             	or     $0xf,%ah
    18d7:	83 c2 01             	add    $0x1,%edx
    18da:	83 c1 02             	add    $0x2,%ecx
    18dd:	66 89 41 fe          	mov    %ax,-0x2(%ecx)
    18e1:	66 0f be 04 13       	movsbw (%ebx,%edx,1),%ax
    18e6:	83 fa 50             	cmp    $0x50,%edx
    18e9:	75 e5                	jne    0x18d0
    18eb:	8b 43 0c             	mov    0xc(%ebx),%eax
    18ee:	85 c0                	test   %eax,%eax
    18f0:	0f 84 ea 00 00 00    	je     0x19e0
    18f6:	8d 7d d4             	lea    -0x2c(%ebp),%edi
    18f9:	89 fa                	mov    %edi,%edx
    18fb:	e8 40 ea ff ff       	call   0x340
    1900:	8b 15 34 59 00 00    	mov    0x5934,%edx
    1906:	66 0f be 45 d4       	movsbw -0x2c(%ebp),%ax
    190b:	8d 14 92             	lea    (%edx,%edx,4),%edx
    190e:	c1 e2 04             	shl    $0x4,%edx
    1911:	84 c0                	test   %al,%al
    1913:	74 26                	je     0x193b
    1915:	8d 8c 12 1a 80 0b 00 	lea    0xb801a(%edx,%edx,1),%ecx
    191c:	31 d2                	xor    %edx,%edx
    191e:	eb 04                	jmp    0x1924
    1920:	84 c0                	test   %al,%al
    1922:	74 17                	je     0x193b
    1924:	80 cc 0e             	or     $0xe,%ah
    1927:	83 c2 01             	add    $0x1,%edx
    192a:	83 c1 02             	add    $0x2,%ecx
    192d:	66 89 41 fe          	mov    %ax,-0x2(%ecx)
    1931:	66 0f be 04 17       	movsbw (%edi,%edx,1),%ax
    1936:	83 fa 43             	cmp    $0x43,%edx
    1939:	75 e5                	jne    0x1920
    193b:	8b 43 10             	mov    0x10(%ebx),%eax
    193e:	85 c0                	test   %eax,%eax
    1940:	74 7e                	je     0x19c0
    1942:	8d 7d de             	lea    -0x22(%ebp),%edi
    1945:	89 fa                	mov    %edi,%edx
    1947:	e8 f4 e9 ff ff       	call   0x340
    194c:	8b 15 34 59 00 00    	mov    0x5934,%edx
    1952:	66 0f be 45 de       	movsbw -0x22(%ebp),%ax
    1957:	8d 14 92             	lea    (%edx,%edx,4),%edx
    195a:	c1 e2 04             	shl    $0x4,%edx
    195d:	84 c0                	test   %al,%al
    195f:	74 2a                	je     0x198b
    1961:	8d 8c 12 28 80 0b 00 	lea    0xb8028(%edx,%edx,1),%ecx
    1968:	31 d2                	xor    %edx,%edx
    196a:	eb 08                	jmp    0x1974
    196c:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    1970:	84 c0                	test   %al,%al
    1972:	74 17                	je     0x198b
    1974:	80 cc 08             	or     $0x8,%ah
    1977:	83 c2 01             	add    $0x1,%edx
    197a:	83 c1 02             	add    $0x2,%ecx
    197d:	66 89 41 fe          	mov    %ax,-0x2(%ecx)
    1981:	66 0f be 04 17       	movsbw (%edi,%edx,1),%ax
    1986:	83 fa 3c             	cmp    $0x3c,%edx
    1989:	75 e5                	jne    0x1970
    198b:	a1 34 59 00 00       	mov    0x5934,%eax
    1990:	83 f8 17             	cmp    $0x17,%eax
    1993:	7f 73                	jg     0x1a08
    1995:	a1 34 59 00 00       	mov    0x5934,%eax
    199a:	83 c0 01             	add    $0x1,%eax
    199d:	a3 34 59 00 00       	mov    %eax,0x5934
    19a2:	83 c3 18             	add    $0x18,%ebx
    19a5:	ba 01 00 00 00       	mov    $0x1,%edx
    19aa:	39 de                	cmp    %ebx,%esi
    19ac:	0f 85 f9 fe ff ff    	jne    0x18ab
    19b2:	85 d2                	test   %edx,%edx
    19b4:	74 73                	je     0x1a29
    19b6:	8d 65 f4             	lea    -0xc(%ebp),%esp
    19b9:	5b                   	pop    %ebx
    19ba:	5e                   	pop    %esi
    19bb:	5f                   	pop    %edi
    19bc:	5d                   	pop    %ebp
    19bd:	c3                   	ret
    19be:	66 90                	xchg   %ax,%ax
    19c0:	0f b7 05 5a 58 00 00 	movzwl 0x585a,%eax
    19c7:	8d 7d de             	lea    -0x22(%ebp),%edi
    19ca:	66 89 45 de          	mov    %ax,-0x22(%ebp)
    19ce:	a1 34 59 00 00       	mov    0x5934,%eax
    19d3:	8d 14 80             	lea    (%eax,%eax,4),%edx
    19d6:	b8 30 00 00 00       	mov    $0x30,%eax
    19db:	c1 e2 04             	shl    $0x4,%edx
    19de:	eb 81                	jmp    0x1961
    19e0:	0f b7 05 5a 58 00 00 	movzwl 0x585a,%eax
    19e7:	8d 7d d4             	lea    -0x2c(%ebp),%edi
    19ea:	66 89 45 d4          	mov    %ax,-0x2c(%ebp)
    19ee:	a1 34 59 00 00       	mov    0x5934,%eax
    19f3:	8d 14 80             	lea    (%eax,%eax,4),%edx
    19f6:	b8 30 00 00 00       	mov    $0x30,%eax
    19fb:	c1 e2 04             	shl    $0x4,%edx
    19fe:	e9 12 ff ff ff       	jmp    0x1915
    1a03:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    1a07:	90                   	nop
    1a08:	e8 93 f7 ff ff       	call   0x11a0
    1a0d:	eb 93                	jmp    0x19a2
    1a0f:	83 ec 08             	sub    $0x8,%esp
    1a12:	68 60 68 00 00       	push   $0x6860
    1a17:	68 96 00 00 00       	push   $0x96
    1a1c:	e8 1f 18 00 00       	call   0x3240
    1a21:	83 c4 10             	add    $0x10,%esp
    1a24:	e9 bd fd ff ff       	jmp    0x17e6
    1a29:	a1 34 59 00 00       	mov    0x5934,%eax
    1a2e:	8d 0c 80             	lea    (%eax,%eax,4),%ecx
    1a31:	b8 4e 00 00 00       	mov    $0x4e,%eax
    1a36:	c1 e1 05             	shl    $0x5,%ecx
    1a39:	81 c1 00 80 0b 00    	add    $0xb8000,%ecx
    1a3f:	eb 0f                	jmp    0x1a50
    1a41:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    1a48:	84 c0                	test   %al,%al
    1a4a:	0f 84 66 ff ff ff    	je     0x19b6
    1a50:	80 cc 0c             	or     $0xc,%ah
    1a53:	83 c2 01             	add    $0x1,%edx
    1a56:	83 c1 02             	add    $0x2,%ecx
    1a59:	66 89 41 fe          	mov    %ax,-0x2(%ecx)
    1a5d:	66 0f be 82 dd 55 00 	movsbw 0x55dd(%edx),%ax
    1a64:	00 
    1a65:	83 fa 50             	cmp    $0x50,%edx
    1a68:	75 de                	jne    0x1a48
    1a6a:	8d 65 f4             	lea    -0xc(%ebp),%esp
    1a6d:	5b                   	pop    %ebx
    1a6e:	5e                   	pop    %esi
    1a6f:	5f                   	pop    %edi
    1a70:	5d                   	pop    %ebp
    1a71:	c3                   	ret
    1a72:	e8 29 f7 ff ff       	call   0x11a0
    1a77:	e9 15 fe ff ff       	jmp    0x1891
    1a7c:	e8 1f f7 ff ff       	call   0x11a0
    1a81:	e9 b3 fd ff ff       	jmp    0x1839
    1a86:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    1a8d:	8d 76 00             	lea    0x0(%esi),%esi
    1a90:	55                   	push   %ebp
    1a91:	89 e5                	mov    %esp,%ebp
    1a93:	57                   	push   %edi
    1a94:	8b 45 0c             	mov    0xc(%ebp),%eax
    1a97:	8b 7d 10             	mov    0x10(%ebp),%edi
    1a9a:	56                   	push   %esi
    1a9b:	53                   	push   %ebx
    1a9c:	8b 5d 08             	mov    0x8(%ebp),%ebx
    1a9f:	8d 04 80             	lea    (%eax,%eax,4),%eax
    1aa2:	8b 75 14             	mov    0x14(%ebp),%esi
    1aa5:	c1 e0 04             	shl    $0x4,%eax
    1aa8:	8d 14 38             	lea    (%eax,%edi,1),%edx
    1aab:	66 0f be 03          	movsbw (%ebx),%ax
    1aaf:	84 c0                	test   %al,%al
    1ab1:	74 34                	je     0x1ae7
    1ab3:	83 ff 4f             	cmp    $0x4f,%edi
    1ab6:	7f 2f                	jg     0x1ae7
    1ab8:	8d 8c 12 00 80 0b 00 	lea    0xb8000(%edx,%edx,1),%ecx
    1abf:	8d 53 01             	lea    0x1(%ebx),%edx
    1ac2:	83 c3 51             	add    $0x51,%ebx
    1ac5:	c1 e6 08             	shl    $0x8,%esi
    1ac8:	29 fb                	sub    %edi,%ebx
    1aca:	eb 0e                	jmp    0x1ada
    1acc:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    1ad0:	83 c2 01             	add    $0x1,%edx
    1ad3:	83 c1 02             	add    $0x2,%ecx
    1ad6:	39 da                	cmp    %ebx,%edx
    1ad8:	74 0d                	je     0x1ae7
    1ada:	09 f0                	or     %esi,%eax
    1adc:	66 89 01             	mov    %ax,(%ecx)
    1adf:	66 0f be 02          	movsbw (%edx),%ax
    1ae3:	84 c0                	test   %al,%al
    1ae5:	75 e9                	jne    0x1ad0
    1ae7:	5b                   	pop    %ebx
    1ae8:	5e                   	pop    %esi
    1ae9:	5f                   	pop    %edi
    1aea:	5d                   	pop    %ebp
    1aeb:	c3                   	ret
    1aec:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    1af0:	31 c0                	xor    %eax,%eax
    1af2:	eb 34                	jmp    0x1b28
    1af4:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    1af8:	83 c2 03             	add    $0x3,%edx
    1afb:	81 fa e7 03 00 00    	cmp    $0x3e7,%edx
    1b01:	7e 33                	jle    0x1b36
    1b03:	89 c2                	mov    %eax,%edx
    1b05:	b9 2a 0a 00 00       	mov    $0xa2a,%ecx
    1b0a:	c7 04 85 a0 6d 00 00 	movl   $0x0,0x6da0(,%eax,4)
    1b11:	00 00 00 00 
    1b15:	83 c0 01             	add    $0x1,%eax
    1b18:	83 e2 03             	and    $0x3,%edx
    1b1b:	66 89 8c 12 96 80 0b 	mov    %cx,0xb8096(%edx,%edx,1)
    1b22:	00 
    1b23:	83 f8 10             	cmp    $0x10,%eax
    1b26:	74 1d                	je     0x1b45
    1b28:	8b 14 85 a0 6d 00 00 	mov    0x6da0(,%eax,4),%edx
    1b2f:	85 d2                	test   %edx,%edx
    1b31:	7f c5                	jg     0x1af8
    1b33:	83 c2 05             	add    $0x5,%edx
    1b36:	89 14 85 a0 6d 00 00 	mov    %edx,0x6da0(,%eax,4)
    1b3d:	83 c0 01             	add    $0x1,%eax
    1b40:	83 f8 10             	cmp    $0x10,%eax
    1b43:	75 e3                	jne    0x1b28
    1b45:	c3                   	ret
    1b46:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    1b4d:	8d 76 00             	lea    0x0(%esi),%esi
    1b50:	55                   	push   %ebp
    1b51:	89 e5                	mov    %esp,%ebp
    1b53:	83 ec 18             	sub    $0x18,%esp
    1b56:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    1b5d:	8d 76 00             	lea    0x0(%esi),%esi
    1b60:	e8 8b ff ff ff       	call   0x1af0
    1b65:	83 ec 0c             	sub    $0xc,%esp
    1b68:	6a 33                	push   $0x33
    1b6a:	68 36 01 00 00       	push   $0x136
    1b6f:	68 20 03 00 00       	push   $0x320
    1b74:	6a 66                	push   $0x66
    1b76:	6a 00                	push   $0x0
    1b78:	e8 13 20 00 00       	call   0x3b90
    1b7d:	83 c4 20             	add    $0x20,%esp
    1b80:	e8 4b f7 ff ff       	call   0x12d0
    1b85:	e8 d6 f9 ff ff       	call   0x1560
    1b8a:	c7 05 60 b0 00 00 00 	movl   $0x0,0xb060
    1b91:	00 00 00 
    1b94:	c7 05 60 59 00 00 38 	movl   $0x138,0x5960
    1b9b:	01 00 00 
    1b9e:	b8 0a 00 00 00       	mov    $0xa,%eax
    1ba3:	cd 80                	int    $0x80
    1ba5:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
    1bac:	8b 45 f4             	mov    -0xc(%ebp),%eax
    1baf:	3d 7f 1a 06 00       	cmp    $0x61a7f,%eax
    1bb4:	7f aa                	jg     0x1b60
    1bb6:	8b 45 f4             	mov    -0xc(%ebp),%eax
    1bb9:	83 c0 01             	add    $0x1,%eax
    1bbc:	89 45 f4             	mov    %eax,-0xc(%ebp)
    1bbf:	8b 45 f4             	mov    -0xc(%ebp),%eax
    1bc2:	3d 7f 1a 06 00       	cmp    $0x61a7f,%eax
    1bc7:	7e ed                	jle    0x1bb6
    1bc9:	eb 95                	jmp    0x1b60
    1bcb:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    1bcf:	90                   	nop
    1bd0:	55                   	push   %ebp
    1bd1:	89 e5                	mov    %esp,%ebp
    1bd3:	53                   	push   %ebx
    1bd4:	83 ec 04             	sub    $0x4,%esp
    1bd7:	a1 2c 6e 00 00       	mov    0x6e2c,%eax
    1bdc:	85 c0                	test   %eax,%eax
    1bde:	75 70                	jne    0x1c50
    1be0:	83 ec 08             	sub    $0x8,%esp
    1be3:	bb 60 68 00 00       	mov    $0x6860,%ebx
    1be8:	68 ff ff 00 00       	push   $0xffff
    1bed:	68 12 56 00 00       	push   $0x5612
    1bf2:	e8 f9 21 00 00       	call   0x3df0
    1bf7:	83 c4 10             	add    $0x10,%esp
    1bfa:	31 c0                	xor    %eax,%eax
    1bfc:	eb 0d                	jmp    0x1c0b
    1bfe:	66 90                	xchg   %ax,%ax
    1c00:	83 c3 18             	add    $0x18,%ebx
    1c03:	81 fb b8 6a 00 00    	cmp    $0x6ab8,%ebx
    1c09:	74 35                	je     0x1c40
    1c0b:	83 7b 14 01          	cmpl   $0x1,0x14(%ebx)
    1c0f:	75 ef                	jne    0x1c00
    1c11:	83 ec 08             	sub    $0x8,%esp
    1c14:	68 00 ff 00 00       	push   $0xff00
    1c19:	53                   	push   %ebx
    1c1a:	83 c3 18             	add    $0x18,%ebx
    1c1d:	e8 ce 21 00 00       	call   0x3df0
    1c22:	58                   	pop    %eax
    1c23:	5a                   	pop    %edx
    1c24:	6a 00                	push   $0x0
    1c26:	68 2c 56 00 00       	push   $0x562c
    1c2b:	e8 c0 21 00 00       	call   0x3df0
    1c30:	83 c4 10             	add    $0x10,%esp
    1c33:	b8 01 00 00 00       	mov    $0x1,%eax
    1c38:	81 fb b8 6a 00 00    	cmp    $0x6ab8,%ebx
    1c3e:	75 cb                	jne    0x1c0b
    1c40:	85 c0                	test   %eax,%eax
    1c42:	74 2c                	je     0x1c70
    1c44:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    1c47:	c9                   	leave
    1c48:	c3                   	ret
    1c49:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    1c50:	83 ec 08             	sub    $0x8,%esp
    1c53:	68 60 68 00 00       	push   $0x6860
    1c58:	68 96 00 00 00       	push   $0x96
    1c5d:	e8 de 15 00 00       	call   0x3240
    1c62:	83 c4 10             	add    $0x10,%esp
    1c65:	e9 76 ff ff ff       	jmp    0x1be0
    1c6a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    1c70:	83 ec 08             	sub    $0x8,%esp
    1c73:	68 00 00 ff 00       	push   $0xff0000
    1c78:	68 2e 56 00 00       	push   $0x562e
    1c7d:	e8 6e 21 00 00       	call   0x3df0
    1c82:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    1c85:	83 c4 10             	add    $0x10,%esp
    1c88:	c9                   	leave
    1c89:	c3                   	ret
    1c8a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    1c90:	55                   	push   %ebp
    1c91:	b8 e0 81 0b 00       	mov    $0xb81e0,%eax
    1c96:	89 e5                	mov    %esp,%ebp
    1c98:	57                   	push   %edi
    1c99:	56                   	push   %esi
    1c9a:	53                   	push   %ebx
    1c9b:	81 ec 1c 02 00 00    	sub    $0x21c,%esp
    1ca1:	8b 5d 08             	mov    0x8(%ebp),%ebx
    1ca4:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    1ca8:	ba 20 4f 00 00       	mov    $0x4f20,%edx
    1cad:	b9 20 4f 00 00       	mov    $0x4f20,%ecx
    1cb2:	83 c0 04             	add    $0x4,%eax
    1cb5:	66 89 50 fc          	mov    %dx,-0x4(%eax)
    1cb9:	66 89 48 fe          	mov    %cx,-0x2(%eax)
    1cbd:	3d 80 82 0b 00       	cmp    $0xb8280,%eax
    1cc2:	75 e4                	jne    0x1ca8
    1cc4:	be 4d 4f 00 00       	mov    $0x4f4d,%esi
    1cc9:	bf 44 4f 00 00       	mov    $0x4f44,%edi
    1cce:	b8 3a 4f 00 00       	mov    $0x4f3a,%eax
    1cd3:	b9 43 4f 00 00       	mov    $0x4f43,%ecx
    1cd8:	66 89 35 e2 81 0b 00 	mov    %si,0xb81e2
    1cdf:	ba e8 81 0b 00       	mov    $0xb81e8,%edx
    1ce4:	66 89 3d e4 81 0b 00 	mov    %di,0xb81e4
    1ceb:	66 a3 e6 81 0b 00    	mov    %ax,0xb81e6
    1cf1:	66 89 0d e0 81 0b 00 	mov    %cx,0xb81e0
    1cf8:	89 d9                	mov    %ebx,%ecx
    1cfa:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    1d00:	66 0f be 01          	movsbw (%ecx),%ax
    1d04:	84 c0                	test   %al,%al
    1d06:	74 15                	je     0x1d1d
    1d08:	80 cc 4f             	or     $0x4f,%ah
    1d0b:	83 c2 02             	add    $0x2,%edx
    1d0e:	83 c1 01             	add    $0x1,%ecx
    1d11:	66 89 42 fe          	mov    %ax,-0x2(%edx)
    1d15:	81 fa fc 81 0b 00    	cmp    $0xb81fc,%edx
    1d1b:	75 e3                	jne    0x1d00
    1d1d:	a1 34 59 00 00       	mov    0x5934,%eax
    1d22:	83 f8 17             	cmp    $0x17,%eax
    1d25:	0f 8f a5 02 00 00    	jg     0x1fd0
    1d2b:	a1 34 59 00 00       	mov    0x5934,%eax
    1d30:	83 c0 01             	add    $0x1,%eax
    1d33:	a3 34 59 00 00       	mov    %eax,0x5934
    1d38:	a1 34 59 00 00       	mov    0x5934,%eax
    1d3d:	8d 3c 80             	lea    (%eax,%eax,4),%edi
    1d40:	0f b6 03             	movzbl (%ebx),%eax
    1d43:	89 fe                	mov    %edi,%esi
    1d45:	c1 e6 04             	shl    $0x4,%esi
    1d48:	3c 73                	cmp    $0x73,%al
    1d4a:	0f 84 f2 00 00 00    	je     0x1e42
    1d50:	3c 6c                	cmp    $0x6c,%al
    1d52:	0f 84 b7 00 00 00    	je     0x1e0f
    1d58:	3c 68                	cmp    $0x68,%al
    1d5a:	0f 84 84 02 00 00    	je     0x1fe4
    1d60:	3c 63                	cmp    $0x63,%al
    1d62:	0f 85 b8 03 00 00    	jne    0x2120
    1d68:	80 7b 01 6c          	cmpb   $0x6c,0x1(%ebx)
    1d6c:	0f 84 9e 04 00 00    	je     0x2210
    1d72:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    1d78:	b8 4e 4f 00 00       	mov    $0x4f4e,%eax
    1d7d:	bb 4e 4f 00 00       	mov    $0x4f4e,%ebx
    1d82:	bf 4f 4f 00 00       	mov    $0x4f4f,%edi
    1d87:	66 a3 0c 82 0b 00    	mov    %ax,0xb820c
    1d8d:	b8 45 4f 00 00       	mov    $0x4f45,%eax
    1d92:	66 a3 0e 82 0b 00    	mov    %ax,0xb820e
    1d98:	b8 3f 4f 00 00       	mov    $0x4f3f,%eax
    1d9d:	66 89 1d 08 82 0b 00 	mov    %bx,0xb8208
    1da4:	66 89 3d 0a 82 0b 00 	mov    %di,0xb820a
    1dab:	c7 84 36 00 80 0b 00 	movl   $0x4f3f4f3f,0xb8000(%esi,%esi,1)
    1db2:	3f 4f 3f 4f 
    1db6:	66 89 84 36 04 80 0b 	mov    %ax,0xb8004(%esi,%esi,1)
    1dbd:	00 
    1dbe:	a1 34 59 00 00       	mov    0x5934,%eax
    1dc3:	83 f8 17             	cmp    $0x17,%eax
    1dc6:	0f 8f 0e 02 00 00    	jg     0x1fda
    1dcc:	a1 34 59 00 00       	mov    0x5934,%eax
    1dd1:	83 c0 01             	add    $0x1,%eax
    1dd4:	a3 34 59 00 00       	mov    %eax,0x5934
    1dd9:	a1 34 59 00 00       	mov    0x5934,%eax
    1dde:	ba 3e 0f 00 00       	mov    $0xf3e,%edx
    1de3:	b9 20 0f 00 00       	mov    $0xf20,%ecx
    1de8:	8d 04 80             	lea    (%eax,%eax,4),%eax
    1deb:	c1 e0 05             	shl    $0x5,%eax
    1dee:	66 89 90 00 80 0b 00 	mov    %dx,0xb8000(%eax)
    1df5:	a1 34 59 00 00       	mov    0x5934,%eax
    1dfa:	8d 04 80             	lea    (%eax,%eax,4),%eax
    1dfd:	c1 e0 05             	shl    $0x5,%eax
    1e00:	66 89 88 02 80 0b 00 	mov    %cx,0xb8002(%eax)
    1e07:	8d 65 f4             	lea    -0xc(%ebp),%esp
    1e0a:	5b                   	pop    %ebx
    1e0b:	5e                   	pop    %esi
    1e0c:	5f                   	pop    %edi
    1e0d:	5d                   	pop    %ebp
    1e0e:	c3                   	ret
    1e0f:	0f b6 43 01          	movzbl 0x1(%ebx),%eax
    1e13:	3c 6f                	cmp    $0x6f,%al
    1e15:	0f 84 9d 06 00 00    	je     0x24b8
    1e1b:	3c 73                	cmp    $0x73,%al
    1e1d:	0f 85 55 ff ff ff    	jne    0x1d78
    1e23:	83 ec 08             	sub    $0x8,%esp
    1e26:	68 ff ff 00 00       	push   $0xffff
    1e2b:	68 3f 56 00 00       	push   $0x563f
    1e30:	e8 bb 1f 00 00       	call   0x3df0
    1e35:	e8 96 fd ff ff       	call   0x1bd0
    1e3a:	83 c4 10             	add    $0x10,%esp
    1e3d:	e9 7c ff ff ff       	jmp    0x1dbe
    1e42:	80 7b 01 61          	cmpb   $0x61,0x1(%ebx)
    1e46:	0f 84 c4 05 00 00    	je     0x2410
    1e4c:	80 7b 01 74          	cmpb   $0x74,0x1(%ebx)
    1e50:	0f 85 22 ff ff ff    	jne    0x1d78
    1e56:	80 7b 02 69          	cmpb   $0x69,0x2(%ebx)
    1e5a:	0f 85 18 ff ff ff    	jne    0x1d78
    1e60:	80 7b 03 6d          	cmpb   $0x6d,0x3(%ebx)
    1e64:	0f 85 0e ff ff ff    	jne    0x1d78
    1e6a:	0f be 73 05          	movsbl 0x5(%ebx),%esi
    1e6e:	0f be 43 07          	movsbl 0x7(%ebx),%eax
    1e72:	b9 f4 01 00 00       	mov    $0x1f4,%ecx
    1e77:	83 ee 30             	sub    $0x30,%esi
    1e7a:	84 c0                	test   %al,%al
    1e7c:	74 1c                	je     0x1e9a
    1e7e:	83 e8 30             	sub    $0x30,%eax
    1e81:	0f be 53 08          	movsbl 0x8(%ebx),%edx
    1e85:	6b c0 64             	imul   $0x64,%eax,%eax
    1e88:	8d 94 92 10 ff ff ff 	lea    -0xf0(%edx,%edx,4),%edx
    1e8f:	8d 14 50             	lea    (%eax,%edx,2),%edx
    1e92:	0f be 43 09          	movsbl 0x9(%ebx),%eax
    1e96:	8d 4c 02 d0          	lea    -0x30(%edx,%eax,1),%ecx
    1e9a:	83 fe 01             	cmp    $0x1,%esi
    1e9d:	0f 87 d6 0a 00 00    	ja     0x2979
    1ea3:	b8 67 66 66 66       	mov    $0x66666667,%eax
    1ea8:	89 8d dc fd ff ff    	mov    %ecx,-0x224(%ebp)
    1eae:	f7 e9                	imul   %ecx
    1eb0:	89 c8                	mov    %ecx,%eax
    1eb2:	c1 f8 1f             	sar    $0x1f,%eax
    1eb5:	d1 fa                	sar    %edx
    1eb7:	29 c2                	sub    %eax,%edx
    1eb9:	89 95 e4 fd ff ff    	mov    %edx,-0x21c(%ebp)
    1ebf:	8d 14 76             	lea    (%esi,%esi,2),%edx
    1ec2:	c1 e2 05             	shl    $0x5,%edx
    1ec5:	8b 9a e8 6b 00 00    	mov    0x6be8(%edx),%ebx
    1ecb:	81 c2 e0 6b 00 00    	add    $0x6be0,%edx
    1ed1:	89 d7                	mov    %edx,%edi
    1ed3:	c1 e3 07             	shl    $0x7,%ebx
    1ed6:	8d b3 18 6d 00 00    	lea    0x6d18(%ebx),%esi
    1edc:	8d 83 a0 6c 00 00    	lea    0x6ca0(%ebx),%eax
    1ee2:	89 b5 e0 fd ff ff    	mov    %esi,-0x220(%ebp)
    1ee8:	8b 70 0c             	mov    0xc(%eax),%esi
    1eeb:	8b 9d e4 fd ff ff    	mov    -0x21c(%ebp),%ebx
    1ef1:	03 18                	add    (%eax),%ebx
    1ef3:	8d 8e 2c 01 00 00    	lea    0x12c(%esi),%ecx
    1ef9:	8b 70 14             	mov    0x14(%eax),%esi
    1efc:	89 18                	mov    %ebx,(%eax)
    1efe:	89 48 0c             	mov    %ecx,0xc(%eax)
    1f01:	8d 96 e0 fc ff ff    	lea    -0x320(%esi),%edx
    1f07:	be c8 00 00 00       	mov    $0xc8,%esi
    1f0c:	89 5f 18             	mov    %ebx,0x18(%edi)
    1f0f:	39 f2                	cmp    %esi,%edx
    1f11:	89 4f 2c             	mov    %ecx,0x2c(%edi)
    1f14:	0f 4c d6             	cmovl  %esi,%edx
    1f17:	83 c0 18             	add    $0x18,%eax
    1f1a:	83 c7 04             	add    $0x4,%edi
    1f1d:	89 50 fc             	mov    %edx,-0x4(%eax)
    1f20:	89 57 3c             	mov    %edx,0x3c(%edi)
    1f23:	39 85 e0 fd ff ff    	cmp    %eax,-0x220(%ebp)
    1f29:	75 bd                	jne    0x1ee8
    1f2b:	a1 34 59 00 00       	mov    0x5934,%eax
    1f30:	8b 8d dc fd ff ff    	mov    -0x224(%ebp),%ecx
    1f36:	31 db                	xor    %ebx,%ebx
    1f38:	ba 53 00 00 00       	mov    $0x53,%edx
    1f3d:	8d 04 80             	lea    (%eax,%eax,4),%eax
    1f40:	c1 e0 05             	shl    $0x5,%eax
    1f43:	05 00 80 0b 00       	add    $0xb8000,%eax
    1f48:	eb 05                	jmp    0x1f4f
    1f4a:	83 fb 50             	cmp    $0x50,%ebx
    1f4d:	74 19                	je     0x1f68
    1f4f:	80 ce 0e             	or     $0xe,%dh
    1f52:	83 c3 01             	add    $0x1,%ebx
    1f55:	83 c0 02             	add    $0x2,%eax
    1f58:	66 89 50 fe          	mov    %dx,-0x2(%eax)
    1f5c:	66 0f be 93 ce 56 00 	movsbw 0x56ce(%ebx),%dx
    1f63:	00 
    1f64:	84 d2                	test   %dl,%dl
    1f66:	75 e2                	jne    0x1f4a
    1f68:	85 c9                	test   %ecx,%ecx
    1f6a:	0f 84 93 0a 00 00    	je     0x2a03
    1f70:	8d 9d e8 fd ff ff    	lea    -0x218(%ebp),%ebx
    1f76:	89 c8                	mov    %ecx,%eax
    1f78:	89 da                	mov    %ebx,%edx
    1f7a:	e8 c1 e3 ff ff       	call   0x340
    1f7f:	8b 15 34 59 00 00    	mov    0x5934,%edx
    1f85:	66 0f be 85 e8 fd ff 	movsbw -0x218(%ebp),%ax
    1f8c:	ff 
    1f8d:	8d 14 92             	lea    (%edx,%edx,4),%edx
    1f90:	c1 e2 04             	shl    $0x4,%edx
    1f93:	84 c0                	test   %al,%al
    1f95:	0f 84 23 fe ff ff    	je     0x1dbe
    1f9b:	8d 8c 12 26 80 0b 00 	lea    0xb8026(%edx,%edx,1),%ecx
    1fa2:	31 d2                	xor    %edx,%edx
    1fa4:	eb 09                	jmp    0x1faf
    1fa6:	83 fa 3d             	cmp    $0x3d,%edx
    1fa9:	0f 84 0f fe ff ff    	je     0x1dbe
    1faf:	80 cc 0f             	or     $0xf,%ah
    1fb2:	83 c2 01             	add    $0x1,%edx
    1fb5:	83 c1 02             	add    $0x2,%ecx
    1fb8:	66 89 41 fe          	mov    %ax,-0x2(%ecx)
    1fbc:	66 0f be 04 13       	movsbw (%ebx,%edx,1),%ax
    1fc1:	84 c0                	test   %al,%al
    1fc3:	75 e1                	jne    0x1fa6
    1fc5:	e9 f4 fd ff ff       	jmp    0x1dbe
    1fca:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    1fd0:	e8 cb f1 ff ff       	call   0x11a0
    1fd5:	e9 5e fd ff ff       	jmp    0x1d38
    1fda:	e8 c1 f1 ff ff       	call   0x11a0
    1fdf:	e9 f5 fd ff ff       	jmp    0x1dd9
    1fe4:	80 7b 01 65          	cmpb   $0x65,0x1(%ebx)
    1fe8:	0f 85 8a fd ff ff    	jne    0x1d78
    1fee:	80 7b 02 6c          	cmpb   $0x6c,0x2(%ebx)
    1ff2:	0f 85 80 fd ff ff    	jne    0x1d78
    1ff8:	80 7b 03 70          	cmpb   $0x70,0x3(%ebx)
    1ffc:	0f 85 76 fd ff ff    	jne    0x1d78
    2002:	80 7b 04 00          	cmpb   $0x0,0x4(%ebx)
    2006:	0f 85 6c fd ff ff    	jne    0x1d78
    200c:	b8 48 07 00 00       	mov    $0x748,%eax
    2011:	31 c9                	xor    %ecx,%ecx
    2013:	ba 73 00 00 00       	mov    $0x73,%edx
    2018:	66 a3 08 82 0b 00    	mov    %ax,0xb8208
    201e:	b8 45 07 00 00       	mov    $0x745,%eax
    2023:	66 a3 0a 82 0b 00    	mov    %ax,0xb820a
    2029:	b8 4c 07 00 00       	mov    $0x74c,%eax
    202e:	66 a3 0c 82 0b 00    	mov    %ax,0xb820c
    2034:	b8 50 07 00 00       	mov    $0x750,%eax
    2039:	66 a3 0e 82 0b 00    	mov    %ax,0xb820e
    203f:	a1 34 59 00 00       	mov    0x5934,%eax
    2044:	8d 04 80             	lea    (%eax,%eax,4),%eax
    2047:	c1 e0 05             	shl    $0x5,%eax
    204a:	05 00 80 0b 00       	add    $0xb8000,%eax
    204f:	eb 04                	jmp    0x2055
    2051:	84 d2                	test   %dl,%dl
    2053:	74 1a                	je     0x206f
    2055:	80 ce 07             	or     $0x7,%dh
    2058:	83 c1 01             	add    $0x1,%ecx
    205b:	83 c0 02             	add    $0x2,%eax
    205e:	66 89 50 fe          	mov    %dx,-0x2(%eax)
    2062:	66 0f be 91 5d 56 00 	movsbw 0x565d(%ecx),%dx
    2069:	00 
    206a:	83 f9 50             	cmp    $0x50,%ecx
    206d:	75 e2                	jne    0x2051
    206f:	a1 34 59 00 00       	mov    0x5934,%eax
    2074:	83 f8 17             	cmp    $0x17,%eax
    2077:	0f 8f 24 0c 00 00    	jg     0x2ca1
    207d:	a1 34 59 00 00       	mov    0x5934,%eax
    2082:	83 c0 01             	add    $0x1,%eax
    2085:	a3 34 59 00 00       	mov    %eax,0x5934
    208a:	a1 34 59 00 00       	mov    0x5934,%eax
    208f:	31 c9                	xor    %ecx,%ecx
    2091:	ba 74 00 00 00       	mov    $0x74,%edx
    2096:	8d 04 80             	lea    (%eax,%eax,4),%eax
    2099:	c1 e0 05             	shl    $0x5,%eax
    209c:	05 00 80 0b 00       	add    $0xb8000,%eax
    20a1:	eb 04                	jmp    0x20a7
    20a3:	84 d2                	test   %dl,%dl
    20a5:	74 1a                	je     0x20c1
    20a7:	80 ce 07             	or     $0x7,%dh
    20aa:	83 c1 01             	add    $0x1,%ecx
    20ad:	83 c0 02             	add    $0x2,%eax
    20b0:	66 89 50 fe          	mov    %dx,-0x2(%eax)
    20b4:	66 0f be 91 f8 57 00 	movsbw 0x57f8(%ecx),%dx
    20bb:	00 
    20bc:	83 f9 50             	cmp    $0x50,%ecx
    20bf:	75 e2                	jne    0x20a3
    20c1:	a1 34 59 00 00       	mov    0x5934,%eax
    20c6:	83 f8 17             	cmp    $0x17,%eax
    20c9:	0f 8f 88 0b 00 00    	jg     0x2c57
    20cf:	a1 34 59 00 00       	mov    0x5934,%eax
    20d4:	83 c0 01             	add    $0x1,%eax
    20d7:	a3 34 59 00 00       	mov    %eax,0x5934
    20dc:	a1 34 59 00 00       	mov    0x5934,%eax
    20e1:	31 c9                	xor    %ecx,%ecx
    20e3:	ba 6d 00 00 00       	mov    $0x6d,%edx
    20e8:	8d 04 80             	lea    (%eax,%eax,4),%eax
    20eb:	c1 e0 05             	shl    $0x5,%eax
    20ee:	05 00 80 0b 00       	add    $0xb8000,%eax
    20f3:	eb 09                	jmp    0x20fe
    20f5:	83 f9 50             	cmp    $0x50,%ecx
    20f8:	0f 84 c0 fc ff ff    	je     0x1dbe
    20fe:	80 ce 07             	or     $0x7,%dh
    2101:	83 c1 01             	add    $0x1,%ecx
    2104:	83 c0 02             	add    $0x2,%eax
    2107:	66 89 50 fe          	mov    %dx,-0x2(%eax)
    210b:	66 0f be 91 78 56 00 	movsbw 0x5678(%ecx),%dx
    2112:	00 
    2113:	84 d2                	test   %dl,%dl
    2115:	75 de                	jne    0x20f5
    2117:	e9 a2 fc ff ff       	jmp    0x1dbe
    211c:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    2120:	3c 65                	cmp    $0x65,%al
    2122:	0f 85 78 01 00 00    	jne    0x22a0
    2128:	80 7b 01 76          	cmpb   $0x76,0x1(%ebx)
    212c:	0f 85 46 fc ff ff    	jne    0x1d78
    2132:	80 7b 02 61          	cmpb   $0x61,0x2(%ebx)
    2136:	0f 85 3c fc ff ff    	jne    0x1d78
    213c:	80 7b 03 6c          	cmpb   $0x6c,0x3(%ebx)
    2140:	0f 85 32 fc ff ff    	jne    0x1d78
    2146:	a1 e8 6b 00 00       	mov    0x6be8,%eax
    214b:	31 c9                	xor    %ecx,%ecx
    214d:	31 db                	xor    %ebx,%ebx
    214f:	c1 e0 07             	shl    $0x7,%eax
    2152:	8d 90 a0 6c 00 00    	lea    0x6ca0(%eax),%edx
    2158:	05 18 6d 00 00       	add    $0x6d18,%eax
    215d:	8b 72 14             	mov    0x14(%edx),%esi
    2160:	03 1a                	add    (%edx),%ebx
    2162:	83 c2 18             	add    $0x18,%edx
    2165:	03 5a f4             	add    -0xc(%edx),%ebx
    2168:	01 ce                	add    %ecx,%esi
    216a:	89 f1                	mov    %esi,%ecx
    216c:	39 d0                	cmp    %edx,%eax
    216e:	75 ed                	jne    0x215d
    2170:	a1 34 59 00 00       	mov    0x5934,%eax
    2175:	31 c9                	xor    %ecx,%ecx
    2177:	ba 4e 00 00 00       	mov    $0x4e,%edx
    217c:	8d 04 80             	lea    (%eax,%eax,4),%eax
    217f:	c1 e0 05             	shl    $0x5,%eax
    2182:	05 00 80 0b 00       	add    $0xb8000,%eax
    2187:	eb 04                	jmp    0x218d
    2189:	84 d2                	test   %dl,%dl
    218b:	74 1a                	je     0x21a7
    218d:	80 ce 0b             	or     $0xb,%dh
    2190:	83 c1 01             	add    $0x1,%ecx
    2193:	83 c0 02             	add    $0x2,%eax
    2196:	66 89 50 fe          	mov    %dx,-0x2(%eax)
    219a:	66 0f be 91 95 56 00 	movsbw 0x5695(%ecx),%dx
    21a1:	00 
    21a2:	83 f9 50             	cmp    $0x50,%ecx
    21a5:	75 e2                	jne    0x2189
    21a7:	a1 34 59 00 00       	mov    0x5934,%eax
    21ac:	83 f8 17             	cmp    $0x17,%eax
    21af:	0f 8f e4 08 00 00    	jg     0x2a99
    21b5:	a1 34 59 00 00       	mov    0x5934,%eax
    21ba:	83 c0 01             	add    $0x1,%eax
    21bd:	a3 34 59 00 00       	mov    %eax,0x5934
    21c2:	a1 34 59 00 00       	mov    0x5934,%eax
    21c7:	31 c9                	xor    %ecx,%ecx
    21c9:	ba 3e 00 00 00       	mov    $0x3e,%edx
    21ce:	8d 04 80             	lea    (%eax,%eax,4),%eax
    21d1:	c1 e0 05             	shl    $0x5,%eax
    21d4:	05 00 80 0b 00       	add    $0xb8000,%eax
    21d9:	39 f3                	cmp    %esi,%ebx
    21db:	0f 8e 97 06 00 00    	jle    0x2878
    21e1:	eb 08                	jmp    0x21eb
    21e3:	84 d2                	test   %dl,%dl
    21e5:	0f 84 d3 fb ff ff    	je     0x1dbe
    21eb:	80 ce 2f             	or     $0x2f,%dh
    21ee:	83 c1 01             	add    $0x1,%ecx
    21f1:	83 c0 02             	add    $0x2,%eax
    21f4:	66 89 50 fe          	mov    %dx,-0x2(%eax)
    21f8:	66 0f be 91 a8 56 00 	movsbw 0x56a8(%ecx),%dx
    21ff:	00 
    2200:	83 f9 50             	cmp    $0x50,%ecx
    2203:	75 de                	jne    0x21e3
    2205:	e9 b4 fb ff ff       	jmp    0x1dbe
    220a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    2210:	80 7b 02 65          	cmpb   $0x65,0x2(%ebx)
    2214:	0f 85 5e fb ff ff    	jne    0x1d78
    221a:	80 7b 03 61          	cmpb   $0x61,0x3(%ebx)
    221e:	0f 85 54 fb ff ff    	jne    0x1d78
    2224:	80 7b 04 72          	cmpb   $0x72,0x4(%ebx)
    2228:	0f 85 4a fb ff ff    	jne    0x1d78
    222e:	80 7b 05 00          	cmpb   $0x0,0x5(%ebx)
    2232:	0f 85 40 fb ff ff    	jne    0x1d78
    2238:	be 43 0a 00 00       	mov    $0xa43,%esi
    223d:	bf 4c 0a 00 00       	mov    $0xa4c,%edi
    2242:	b8 52 0a 00 00       	mov    $0xa52,%eax
    2247:	ba 00 8a 0b 00       	mov    $0xb8a00,%edx
    224c:	66 89 35 08 82 0b 00 	mov    %si,0xb8208
    2253:	66 89 3d 0a 82 0b 00 	mov    %di,0xb820a
    225a:	66 a3 0c 82 0b 00    	mov    %ax,0xb820c
    2260:	8d 82 60 ff ff ff    	lea    -0xa0(%edx),%eax
    2266:	b9 20 0f 00 00       	mov    $0xf20,%ecx
    226b:	bb 20 0f 00 00       	mov    $0xf20,%ebx
    2270:	83 c0 04             	add    $0x4,%eax
    2273:	66 89 48 fc          	mov    %cx,-0x4(%eax)
    2277:	66 89 58 fe          	mov    %bx,-0x2(%eax)
    227b:	39 c2                	cmp    %eax,%edx
    227d:	75 e7                	jne    0x2266
    227f:	81 c2 a0 00 00 00    	add    $0xa0,%edx
    2285:	81 fa 40 90 0b 00    	cmp    $0xb9040,%edx
    228b:	75 d3                	jne    0x2260
    228d:	c7 05 34 59 00 00 0f 	movl   $0xf,0x5934
    2294:	00 00 00 
    2297:	e9 6b fb ff ff       	jmp    0x1e07
    229c:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    22a0:	3c 6d                	cmp    $0x6d,%al
    22a2:	0f 85 98 00 00 00    	jne    0x2340
    22a8:	80 7b 01 61          	cmpb   $0x61,0x1(%ebx)
    22ac:	0f 85 c6 fa ff ff    	jne    0x1d78
    22b2:	0f b6 43 02          	movzbl 0x2(%ebx),%eax
    22b6:	3c 6c                	cmp    $0x6c,%al
    22b8:	0f 84 e2 05 00 00    	je     0x28a0
    22be:	3c 70                	cmp    $0x70,%al
    22c0:	0f 85 b2 fa ff ff    	jne    0x1d78
    22c6:	a1 34 59 00 00       	mov    0x5934,%eax
    22cb:	31 c9                	xor    %ecx,%ecx
    22cd:	8d 14 80             	lea    (%eax,%eax,4),%edx
    22d0:	b8 2d 00 00 00       	mov    $0x2d,%eax
    22d5:	c1 e2 05             	shl    $0x5,%edx
    22d8:	81 c2 00 80 0b 00    	add    $0xb8000,%edx
    22de:	eb 04                	jmp    0x22e4
    22e0:	84 c0                	test   %al,%al
    22e2:	74 1a                	je     0x22fe
    22e4:	80 cc 0b             	or     $0xb,%ah
    22e7:	83 c1 01             	add    $0x1,%ecx
    22ea:	83 c2 02             	add    $0x2,%edx
    22ed:	66 89 42 fe          	mov    %ax,-0x2(%edx)
    22f1:	66 0f be 81 26 57 00 	movsbw 0x5726(%ecx),%ax
    22f8:	00 
    22f9:	83 f9 50             	cmp    $0x50,%ecx
    22fc:	75 e2                	jne    0x22e0
    22fe:	a1 34 59 00 00       	mov    0x5934,%eax
    2303:	83 c0 01             	add    $0x1,%eax
    2306:	a3 34 59 00 00       	mov    %eax,0x5934
    230b:	a1 34 59 00 00       	mov    0x5934,%eax
    2310:	83 f8 17             	cmp    $0x17,%eax
    2313:	0f 8f a0 06 00 00    	jg     0x29b9
    2319:	e8 e2 11 00 00       	call   0x3500
    231e:	a1 34 59 00 00       	mov    0x5934,%eax
    2323:	83 f8 17             	cmp    $0x17,%eax
    2326:	0f 8e 92 fa ff ff    	jle    0x1dbe
    232c:	e8 6f ee ff ff       	call   0x11a0
    2331:	e9 88 fa ff ff       	jmp    0x1dbe
    2336:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    233d:	8d 76 00             	lea    0x0(%esi),%esi
    2340:	3c 66                	cmp    $0x66,%al
    2342:	0f 85 58 02 00 00    	jne    0x25a0
    2348:	80 7b 01 72          	cmpb   $0x72,0x1(%ebx)
    234c:	0f 85 26 fa ff ff    	jne    0x1d78
    2352:	80 7b 02 65          	cmpb   $0x65,0x2(%ebx)
    2356:	0f 85 1c fa ff ff    	jne    0x1d78
    235c:	80 7b 03 65          	cmpb   $0x65,0x3(%ebx)
    2360:	0f 85 12 fa ff ff    	jne    0x1d78
    2366:	83 ec 0c             	sub    $0xc,%esp
    2369:	8d 43 05             	lea    0x5(%ebx),%eax
    236c:	50                   	push   %eax
    236d:	e8 de ee ff ff       	call   0x1250
    2372:	83 c4 10             	add    $0x10,%esp
    2375:	3d ff ff 0f 00       	cmp    $0xfffff,%eax
    237a:	0f 86 43 06 00 00    	jbe    0x29c3
    2380:	83 ec 0c             	sub    $0xc,%esp
    2383:	50                   	push   %eax
    2384:	e8 47 10 00 00       	call   0x33d0
    2389:	a1 34 59 00 00       	mov    0x5934,%eax
    238e:	83 c4 10             	add    $0x10,%esp
    2391:	31 c9                	xor    %ecx,%ecx
    2393:	ba 50 00 00 00       	mov    $0x50,%edx
    2398:	8d 04 80             	lea    (%eax,%eax,4),%eax
    239b:	c1 e0 05             	shl    $0x5,%eax
    239e:	05 00 80 0b 00       	add    $0xb8000,%eax
    23a3:	eb 04                	jmp    0x23a9
    23a5:	84 d2                	test   %dl,%dl
    23a7:	74 1a                	je     0x23c3
    23a9:	80 ce 0e             	or     $0xe,%dh
    23ac:	83 c1 01             	add    $0x1,%ecx
    23af:	83 c0 02             	add    $0x2,%eax
    23b2:	66 89 50 fe          	mov    %dx,-0x2(%eax)
    23b6:	66 0f be 91 13 57 00 	movsbw 0x5713(%ecx),%dx
    23bd:	00 
    23be:	83 f9 50             	cmp    $0x50,%ecx
    23c1:	75 e2                	jne    0x23a5
    23c3:	a1 34 59 00 00       	mov    0x5934,%eax
    23c8:	8d 14 80             	lea    (%eax,%eax,4),%edx
    23cb:	66 0f be 43 05       	movsbw 0x5(%ebx),%ax
    23d0:	c1 e2 04             	shl    $0x4,%edx
    23d3:	83 c2 12             	add    $0x12,%edx
    23d6:	84 c0                	test   %al,%al
    23d8:	0f 84 e0 f9 ff ff    	je     0x1dbe
    23de:	8d 8c 12 00 80 0b 00 	lea    0xb8000(%edx,%edx,1),%ecx
    23e5:	31 d2                	xor    %edx,%edx
    23e7:	eb 08                	jmp    0x23f1
    23e9:	84 c0                	test   %al,%al
    23eb:	0f 84 cd f9 ff ff    	je     0x1dbe
    23f1:	80 cc 0f             	or     $0xf,%ah
    23f4:	83 c2 01             	add    $0x1,%edx
    23f7:	83 c1 02             	add    $0x2,%ecx
    23fa:	66 89 41 fe          	mov    %ax,-0x2(%ecx)
    23fe:	66 0f be 44 13 05    	movsbw 0x5(%ebx,%edx,1),%ax
    2404:	83 fa 3e             	cmp    $0x3e,%edx
    2407:	75 e0                	jne    0x23e9
    2409:	e9 b0 f9 ff ff       	jmp    0x1dbe
    240e:	66 90                	xchg   %ax,%ax
    2410:	80 7b 02 76          	cmpb   $0x76,0x2(%ebx)
    2414:	0f 85 5e f9 ff ff    	jne    0x1d78
    241a:	80 7b 03 65          	cmpb   $0x65,0x3(%ebx)
    241e:	0f 85 54 f9 ff ff    	jne    0x1d78
    2424:	80 7b 04 20          	cmpb   $0x20,0x4(%ebx)
    2428:	0f 85 4a f9 ff ff    	jne    0x1d78
    242e:	0f be 43 05          	movsbl 0x5(%ebx),%eax
    2432:	ba 53 2f 00 00       	mov    $0x2f53,%edx
    2437:	bb 56 2f 00 00       	mov    $0x2f56,%ebx
    243c:	c1 e7 05             	shl    $0x5,%edi
    243f:	66 89 15 08 82 0b 00 	mov    %dx,0xb8208
    2446:	b9 41 2f 00 00       	mov    $0x2f41,%ecx
    244b:	ba 45 2f 00 00       	mov    $0x2f45,%edx
    2450:	83 e8 30             	sub    $0x30,%eax
    2453:	66 89 1d 0c 82 0b 00 	mov    %bx,0xb820c
    245a:	8d 5c 36 02          	lea    0x2(%esi,%esi,1),%ebx
    245e:	66 89 0d 0a 82 0b 00 	mov    %cx,0xb820a
    2465:	66 89 15 0e 82 0b 00 	mov    %dx,0xb820e
    246c:	83 f8 01             	cmp    $0x1,%eax
    246f:	0f 86 36 08 00 00    	jbe    0x2cab
    2475:	b9 45 4f 00 00       	mov    $0x4f45,%ecx
    247a:	b8 45 4f 00 00       	mov    $0x4f45,%eax
    247f:	be 52 4f 00 00       	mov    $0x4f52,%esi
    2484:	66 89 8f 00 80 0b 00 	mov    %cx,0xb8000(%edi)
    248b:	bf 52 4f 00 00       	mov    $0x4f52,%edi
    2490:	66 89 b3 00 80 0b 00 	mov    %si,0xb8000(%ebx)
    2497:	66 89 bb 02 80 0b 00 	mov    %di,0xb8002(%ebx)
    249e:	66 a3 12 82 0b 00    	mov    %ax,0xb8212
    24a4:	b8 52 4f 00 00       	mov    $0x4f52,%eax
    24a9:	66 a3 14 82 0b 00    	mov    %ax,0xb8214
    24af:	e9 0a f9 ff ff       	jmp    0x1dbe
    24b4:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    24b8:	80 7b 02 61          	cmpb   $0x61,0x2(%ebx)
    24bc:	0f 85 b6 f8 ff ff    	jne    0x1d78
    24c2:	80 7b 03 64          	cmpb   $0x64,0x3(%ebx)
    24c6:	0f 85 ac f8 ff ff    	jne    0x1d78
    24cc:	80 7b 04 20          	cmpb   $0x20,0x4(%ebx)
    24d0:	0f 85 a2 f8 ff ff    	jne    0x1d78
    24d6:	0f be 43 05          	movsbl 0x5(%ebx),%eax
    24da:	ba 4c 1f 00 00       	mov    $0x1f4c,%edx
    24df:	b9 4f 1f 00 00       	mov    $0x1f4f,%ecx
    24e4:	bb 41 1f 00 00       	mov    $0x1f41,%ebx
    24e9:	66 89 15 08 82 0b 00 	mov    %dx,0xb8208
    24f0:	ba 44 1f 00 00       	mov    $0x1f44,%edx
    24f5:	83 e8 30             	sub    $0x30,%eax
    24f8:	66 89 0d 0a 82 0b 00 	mov    %cx,0xb820a
    24ff:	66 89 1d 0c 82 0b 00 	mov    %bx,0xb820c
    2506:	66 89 15 0e 82 0b 00 	mov    %dx,0xb820e
    250d:	83 f8 01             	cmp    $0x1,%eax
    2510:	0f 87 a8 f8 ff ff    	ja     0x1dbe
    2516:	83 ec 08             	sub    $0x8,%esp
    2519:	c1 e7 05             	shl    $0x5,%edi
    251c:	50                   	push   %eax
    251d:	50                   	push   %eax
    251e:	e8 2d eb ff ff       	call   0x1050
    2523:	83 c4 10             	add    $0x10,%esp
    2526:	89 c2                	mov    %eax,%edx
    2528:	8d 87 00 80 0b 00    	lea    0xb8000(%edi),%eax
    252e:	85 d2                	test   %edx,%edx
    2530:	0f 84 e0 07 00 00    	je     0x2d16
    2536:	83 c6 01             	add    $0x1,%esi
    2539:	ba 4f 2f 00 00       	mov    $0x2f4f,%edx
    253e:	b8 4b 2f 00 00       	mov    $0x2f4b,%eax
    2543:	66 c7 87 00 80 0b 00 	movw   $0x2f4c,0xb8000(%edi)
    254a:	4c 2f 
    254c:	01 f6                	add    %esi,%esi
    254e:	66 c7 86 00 80 0b 00 	movw   $0x2f4f,0xb8000(%esi)
    2555:	4f 2f 
    2557:	66 c7 86 02 80 0b 00 	movw   $0x2f41,0xb8002(%esi)
    255e:	41 2f 
    2560:	66 c7 86 04 80 0b 00 	movw   $0x2f44,0xb8004(%esi)
    2567:	44 2f 
    2569:	66 c7 86 06 80 0b 00 	movw   $0x2f45,0xb8006(%esi)
    2570:	45 2f 
    2572:	66 c7 86 08 80 0b 00 	movw   $0x2f44,0xb8008(%esi)
    2579:	44 2f 
    257b:	66 c7 86 0a 80 0b 00 	movw   $0x2f21,0xb800a(%esi)
    2582:	21 2f 
    2584:	66 89 15 12 82 0b 00 	mov    %dx,0xb8212
    258b:	66 a3 14 82 0b 00    	mov    %ax,0xb8214
    2591:	e9 28 f8 ff ff       	jmp    0x1dbe
    2596:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    259d:	8d 76 00             	lea    0x0(%esi),%esi
    25a0:	3c 74                	cmp    $0x74,%al
    25a2:	0f 85 08 01 00 00    	jne    0x26b0
    25a8:	0f b6 43 01          	movzbl 0x1(%ebx),%eax
    25ac:	3c 73                	cmp    $0x73,%al
    25ae:	0f 84 22 01 00 00    	je     0x26d6
    25b4:	3c 6c                	cmp    $0x6c,%al
    25b6:	0f 85 bc f7 ff ff    	jne    0x1d78
    25bc:	80 7b 02 6f          	cmpb   $0x6f,0x2(%ebx)
    25c0:	0f 85 b2 f7 ff ff    	jne    0x1d78
    25c6:	80 7b 03 61          	cmpb   $0x61,0x3(%ebx)
    25ca:	0f 85 a8 f7 ff ff    	jne    0x1d78
    25d0:	80 7b 04 64          	cmpb   $0x64,0x4(%ebx)
    25d4:	0f 85 9e f7 ff ff    	jne    0x1d78
    25da:	80 7b 05 20          	cmpb   $0x20,0x5(%ebx)
    25de:	0f 85 c8 07 00 00    	jne    0x2dac
    25e4:	80 7b 06 00          	cmpb   $0x0,0x6(%ebx)
    25e8:	0f 84 be 07 00 00    	je     0x2dac
    25ee:	a1 2c 6e 00 00       	mov    0x6e2c,%eax
    25f3:	85 c0                	test   %eax,%eax
    25f5:	0f 85 98 07 00 00    	jne    0x2d93
    25fb:	bf 6b 68 00 00       	mov    $0x686b,%edi
    2600:	31 d2                	xor    %edx,%edx
    2602:	eb 0f                	jmp    0x2613
    2604:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    2608:	83 c2 01             	add    $0x1,%edx
    260b:	83 c7 18             	add    $0x18,%edi
    260e:	83 fa 19             	cmp    $0x19,%edx
    2611:	74 7a                	je     0x268d
    2613:	83 7f 09 01          	cmpl   $0x1,0x9(%edi)
    2617:	75 ef                	jne    0x2608
    2619:	8d 43 06             	lea    0x6(%ebx),%eax
    261c:	89 5d 08             	mov    %ebx,0x8(%ebp)
    261f:	8d 77 f5             	lea    -0xb(%edi),%esi
    2622:	89 d3                	mov    %edx,%ebx
    2624:	89 85 e4 fd ff ff    	mov    %eax,-0x21c(%ebp)
    262a:	eb 1d                	jmp    0x2649
    262c:	83 c2 20             	add    $0x20,%edx
    262f:	38 c2                	cmp    %al,%dl
    2631:	0f 85 cc 05 00 00    	jne    0x2c03
    2637:	83 c6 01             	add    $0x1,%esi
    263a:	83 85 e4 fd ff ff 01 	addl   $0x1,-0x21c(%ebp)
    2641:	39 f7                	cmp    %esi,%edi
    2643:	0f 84 b3 05 00 00    	je     0x2bfc
    2649:	0f b6 06             	movzbl (%esi),%eax
    264c:	8b 8d e4 fd ff ff    	mov    -0x21c(%ebp),%ecx
    2652:	0f b6 11             	movzbl (%ecx),%edx
    2655:	8d 48 bf             	lea    -0x41(%eax),%ecx
    2658:	88 8d e0 fd ff ff    	mov    %cl,-0x220(%ebp)
    265e:	8d 48 20             	lea    0x20(%eax),%ecx
    2661:	80 bd e0 fd ff ff 1a 	cmpb   $0x1a,-0x220(%ebp)
    2668:	0f 42 c1             	cmovb  %ecx,%eax
    266b:	8d 4a bf             	lea    -0x41(%edx),%ecx
    266e:	80 f9 19             	cmp    $0x19,%cl
    2671:	76 b9                	jbe    0x262c
    2673:	84 d2                	test   %dl,%dl
    2675:	75 b8                	jne    0x262f
    2677:	89 da                	mov    %ebx,%edx
    2679:	8b 5d 08             	mov    0x8(%ebp),%ebx
    267c:	84 c0                	test   %al,%al
    267e:	75 88                	jne    0x2608
    2680:	a1 2c 6e 00 00       	mov    0x6e2c,%eax
    2685:	85 c0                	test   %eax,%eax
    2687:	0f 85 80 05 00 00    	jne    0x2c0d
    268d:	a1 34 59 00 00       	mov    0x5934,%eax
    2692:	6a 0c                	push   $0xc
    2694:	6a 00                	push   $0x0
    2696:	50                   	push   %eax
    2697:	68 8d 57 00 00       	push   $0x578d
    269c:	e8 ef f3 ff ff       	call   0x1a90
    26a1:	83 c4 10             	add    $0x10,%esp
    26a4:	e9 15 f7 ff ff       	jmp    0x1dbe
    26a9:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    26b0:	3c 70                	cmp    $0x70,%al
    26b2:	0f 85 eb 03 00 00    	jne    0x2aa3
    26b8:	80 7b 01 63          	cmpb   $0x63,0x1(%ebx)
    26bc:	0f 85 b6 f6 ff ff    	jne    0x1d78
    26c2:	80 7b 02 69          	cmpb   $0x69,0x2(%ebx)
    26c6:	0f 85 ac f6 ff ff    	jne    0x1d78
    26cc:	e8 ff 1a 00 00       	call   0x41d0
    26d1:	e9 e8 f6 ff ff       	jmp    0x1dbe
    26d6:	80 7b 02 61          	cmpb   $0x61,0x2(%ebx)
    26da:	0f 85 98 f6 ff ff    	jne    0x1d78
    26e0:	80 7b 03 76          	cmpb   $0x76,0x3(%ebx)
    26e4:	0f 85 8e f6 ff ff    	jne    0x1d78
    26ea:	80 7b 04 65          	cmpb   $0x65,0x4(%ebx)
    26ee:	0f 85 84 f6 ff ff    	jne    0x1d78
    26f4:	a1 2c 6e 00 00       	mov    0x6e2c,%eax
    26f9:	85 c0                	test   %eax,%eax
    26fb:	0f 85 78 06 00 00    	jne    0x2d79
    2701:	83 ec 0c             	sub    $0xc,%esp
    2704:	68 60 68 00 00       	push   $0x6860
    2709:	e8 e2 0b 00 00       	call   0x32f0
    270e:	83 c4 10             	add    $0x10,%esp
    2711:	31 c9                	xor    %ecx,%ecx
    2713:	89 c6                	mov    %eax,%esi
    2715:	a1 34 59 00 00       	mov    0x5934,%eax
    271a:	8d 04 80             	lea    (%eax,%eax,4),%eax
    271d:	c1 e0 05             	shl    $0x5,%eax
    2720:	05 00 80 0b 00       	add    $0xb8000,%eax
    2725:	83 fe ff             	cmp    $0xffffffff,%esi
    2728:	0f 84 1d 06 00 00    	je     0x2d4b
    272e:	ba 53 00 00 00       	mov    $0x53,%edx
    2733:	eb 04                	jmp    0x2739
    2735:	84 d2                	test   %dl,%dl
    2737:	74 1a                	je     0x2753
    2739:	80 ce 0e             	or     $0xe,%dh
    273c:	83 c1 01             	add    $0x1,%ecx
    273f:	83 c0 02             	add    $0x2,%eax
    2742:	66 89 50 fe          	mov    %dx,-0x2(%eax)
    2746:	66 0f be 91 39 57 00 	movsbw 0x5739(%ecx),%dx
    274d:	00 
    274e:	83 f9 50             	cmp    $0x50,%ecx
    2751:	75 e2                	jne    0x2735
    2753:	a1 34 59 00 00       	mov    0x5934,%eax
    2758:	83 f8 17             	cmp    $0x17,%eax
    275b:	0f 8f 2e 03 00 00    	jg     0x2a8f
    2761:	a1 34 59 00 00       	mov    0x5934,%eax
    2766:	83 c0 01             	add    $0x1,%eax
    2769:	a3 34 59 00 00       	mov    %eax,0x5934
    276e:	8d 3c 36             	lea    (%esi,%esi,1),%edi
    2771:	8d 8e c8 00 00 00    	lea    0xc8(%esi),%ecx
    2777:	8d 14 37             	lea    (%edi,%esi,1),%edx
    277a:	c1 e2 03             	shl    $0x3,%edx
    277d:	89 8a 6c 68 00 00    	mov    %ecx,0x686c(%edx)
    2783:	c7 82 74 68 00 00 01 	movl   $0x1,0x6874(%edx)
    278a:	00 00 00 
    278d:	c7 82 70 68 00 00 40 	movl   $0x40,0x6870(%edx)
    2794:	00 00 00 
    2797:	80 7b 05 20          	cmpb   $0x20,0x5(%ebx)
    279b:	0f 84 af 02 00 00    	je     0x2a50
    27a1:	8d 04 37             	lea    (%edi,%esi,1),%eax
    27a4:	8d 56 30             	lea    0x30(%esi),%edx
    27a7:	c1 e0 03             	shl    $0x3,%eax
    27aa:	c7 80 60 68 00 00 62 	movl   $0x69617262,0x6860(%eax)
    27b1:	72 61 69 
    27b4:	c6 80 64 68 00 00 6e 	movb   $0x6e,0x6864(%eax)
    27bb:	88 90 65 68 00 00    	mov    %dl,0x6865(%eax)
    27c1:	c6 80 66 68 00 00 00 	movb   $0x0,0x6866(%eax)
    27c8:	a1 2c 6e 00 00       	mov    0x6e2c,%eax
    27cd:	85 c0                	test   %eax,%eax
    27cf:	0f 85 55 02 00 00    	jne    0x2a2a
    27d5:	a1 34 59 00 00       	mov    0x5934,%eax
    27da:	31 c9                	xor    %ecx,%ecx
    27dc:	ba 53 00 00 00       	mov    $0x53,%edx
    27e1:	8d 04 80             	lea    (%eax,%eax,4),%eax
    27e4:	c1 e0 05             	shl    $0x5,%eax
    27e7:	05 00 80 0b 00       	add    $0xb8000,%eax
    27ec:	eb 04                	jmp    0x27f2
    27ee:	84 d2                	test   %dl,%dl
    27f0:	74 1a                	je     0x280c
    27f2:	80 ce 0a             	or     $0xa,%dh
    27f5:	83 c1 01             	add    $0x1,%ecx
    27f8:	83 c0 02             	add    $0x2,%eax
    27fb:	66 89 50 fe          	mov    %dx,-0x2(%eax)
    27ff:	66 0f be 91 4c 57 00 	movsbw 0x574c(%ecx),%dx
    2806:	00 
    2807:	83 f9 50             	cmp    $0x50,%ecx
    280a:	75 e2                	jne    0x27ee
    280c:	8b 15 34 59 00 00    	mov    0x5934,%edx
    2812:	8d 04 37             	lea    (%edi,%esi,1),%eax
    2815:	c1 e0 03             	shl    $0x3,%eax
    2818:	8d 88 60 68 00 00    	lea    0x6860(%eax),%ecx
    281e:	8d 14 92             	lea    (%edx,%edx,4),%edx
    2821:	66 0f be 80 60 68 00 	movsbw 0x6860(%eax),%ax
    2828:	00 
    2829:	c1 e2 04             	shl    $0x4,%edx
    282c:	83 c2 0a             	add    $0xa,%edx
    282f:	84 c0                	test   %al,%al
    2831:	0f 84 87 f5 ff ff    	je     0x1dbe
    2837:	8d 9c 12 00 80 0b 00 	lea    0xb8000(%edx,%edx,1),%ebx
    283e:	31 d2                	xor    %edx,%edx
    2840:	eb 08                	jmp    0x284a
    2842:	84 c0                	test   %al,%al
    2844:	0f 84 74 f5 ff ff    	je     0x1dbe
    284a:	80 cc 0f             	or     $0xf,%ah
    284d:	83 c2 01             	add    $0x1,%edx
    2850:	83 c3 02             	add    $0x2,%ebx
    2853:	66 89 43 fe          	mov    %ax,-0x2(%ebx)
    2857:	66 0f be 04 11       	movsbw (%ecx,%edx,1),%ax
    285c:	83 fa 46             	cmp    $0x46,%edx
    285f:	75 e1                	jne    0x2842
    2861:	e9 58 f5 ff ff       	jmp    0x1dbe
    2866:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    286d:	8d 76 00             	lea    0x0(%esi),%esi
    2870:	84 d2                	test   %dl,%dl
    2872:	0f 84 46 f5 ff ff    	je     0x1dbe
    2878:	80 ce 4e             	or     $0x4e,%dh
    287b:	83 c1 01             	add    $0x1,%ecx
    287e:	83 c0 02             	add    $0x2,%eax
    2881:	66 89 50 fe          	mov    %dx,-0x2(%eax)
    2885:	66 0f be 91 bb 56 00 	movsbw 0x56bb(%ecx),%dx
    288c:	00 
    288d:	83 f9 50             	cmp    $0x50,%ecx
    2890:	75 de                	jne    0x2870
    2892:	e9 27 f5 ff ff       	jmp    0x1dbe
    2897:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    289e:	66 90                	xchg   %ax,%ax
    28a0:	80 7b 03 6c          	cmpb   $0x6c,0x3(%ebx)
    28a4:	0f 85 ce f4 ff ff    	jne    0x1d78
    28aa:	e8 71 0b 00 00       	call   0x3420
    28af:	85 c0                	test   %eax,%eax
    28b1:	0f 84 aa 03 00 00    	je     0x2c61
    28b7:	be 30 78 00 00       	mov    $0x7830,%esi
    28bc:	8d 95 f1 fd ff ff    	lea    -0x20f(%ebp),%edx
    28c2:	8d 8d e9 fd ff ff    	lea    -0x217(%ebp),%ecx
    28c8:	66 89 b5 e8 fd ff ff 	mov    %si,-0x218(%ebp)
    28cf:	89 c3                	mov    %eax,%ebx
    28d1:	83 ea 01             	sub    $0x1,%edx
    28d4:	c1 e8 04             	shr    $0x4,%eax
    28d7:	83 e3 0f             	and    $0xf,%ebx
    28da:	0f b6 9b 0e 55 00 00 	movzbl 0x550e(%ebx),%ebx
    28e1:	88 5a 01             	mov    %bl,0x1(%edx)
    28e4:	39 d1                	cmp    %edx,%ecx
    28e6:	75 e7                	jne    0x28cf
    28e8:	a1 34 59 00 00       	mov    0x5934,%eax
    28ed:	c6 85 f2 fd ff ff 00 	movb   $0x0,-0x20e(%ebp)
    28f4:	31 c9                	xor    %ecx,%ecx
    28f6:	ba 41 00 00 00       	mov    $0x41,%edx
    28fb:	8d 04 80             	lea    (%eax,%eax,4),%eax
    28fe:	c1 e0 05             	shl    $0x5,%eax
    2901:	05 00 80 0b 00       	add    $0xb8000,%eax
    2906:	eb 04                	jmp    0x290c
    2908:	84 d2                	test   %dl,%dl
    290a:	74 1a                	je     0x2926
    290c:	80 ce 0a             	or     $0xa,%dh
    290f:	83 c1 01             	add    $0x1,%ecx
    2912:	83 c0 02             	add    $0x2,%eax
    2915:	66 89 50 fe          	mov    %dx,-0x2(%eax)
    2919:	66 0f be 91 18 58 00 	movsbw 0x5818(%ecx),%dx
    2920:	00 
    2921:	83 f9 50             	cmp    $0x50,%ecx
    2924:	75 e2                	jne    0x2908
    2926:	a1 34 59 00 00       	mov    0x5934,%eax
    292b:	8d 14 80             	lea    (%eax,%eax,4),%edx
    292e:	66 0f be 85 e8 fd ff 	movsbw -0x218(%ebp),%ax
    2935:	ff 
    2936:	c1 e2 04             	shl    $0x4,%edx
    2939:	83 c2 1e             	add    $0x1e,%edx
    293c:	84 c0                	test   %al,%al
    293e:	0f 84 7a f4 ff ff    	je     0x1dbe
    2944:	8d 8c 12 00 80 0b 00 	lea    0xb8000(%edx,%edx,1),%ecx
    294b:	8d 9d e8 fd ff ff    	lea    -0x218(%ebp),%ebx
    2951:	31 d2                	xor    %edx,%edx
    2953:	eb 09                	jmp    0x295e
    2955:	83 fa 32             	cmp    $0x32,%edx
    2958:	0f 84 60 f4 ff ff    	je     0x1dbe
    295e:	80 cc 0f             	or     $0xf,%ah
    2961:	83 c2 01             	add    $0x1,%edx
    2964:	83 c1 02             	add    $0x2,%ecx
    2967:	66 89 41 fe          	mov    %ax,-0x2(%ecx)
    296b:	66 0f be 04 13       	movsbw (%ebx,%edx,1),%ax
    2970:	84 c0                	test   %al,%al
    2972:	75 e1                	jne    0x2955
    2974:	e9 45 f4 ff ff       	jmp    0x1dbe
    2979:	a1 34 59 00 00       	mov    0x5934,%eax
    297e:	31 c9                	xor    %ecx,%ecx
    2980:	ba 45 00 00 00       	mov    $0x45,%edx
    2985:	8d 04 80             	lea    (%eax,%eax,4),%eax
    2988:	c1 e0 05             	shl    $0x5,%eax
    298b:	05 00 80 0b 00       	add    $0xb8000,%eax
    2990:	eb 09                	jmp    0x299b
    2992:	83 f9 50             	cmp    $0x50,%ecx
    2995:	0f 84 23 f4 ff ff    	je     0x1dbe
    299b:	80 ce 0c             	or     $0xc,%dh
    299e:	83 c1 01             	add    $0x1,%ecx
    29a1:	83 c0 02             	add    $0x2,%eax
    29a4:	66 89 50 fe          	mov    %dx,-0x2(%eax)
    29a8:	66 0f be 91 e2 56 00 	movsbw 0x56e2(%ecx),%dx
    29af:	00 
    29b0:	84 d2                	test   %dl,%dl
    29b2:	75 de                	jne    0x2992
    29b4:	e9 05 f4 ff ff       	jmp    0x1dbe
    29b9:	e8 e2 e7 ff ff       	call   0x11a0
    29be:	e9 56 f9 ff ff       	jmp    0x2319
    29c3:	a1 34 59 00 00       	mov    0x5934,%eax
    29c8:	31 c9                	xor    %ecx,%ecx
    29ca:	ba 45 00 00 00       	mov    $0x45,%edx
    29cf:	8d 04 80             	lea    (%eax,%eax,4),%eax
    29d2:	c1 e0 05             	shl    $0x5,%eax
    29d5:	05 00 80 0b 00       	add    $0xb8000,%eax
    29da:	eb 08                	jmp    0x29e4
    29dc:	84 d2                	test   %dl,%dl
    29de:	0f 84 da f3 ff ff    	je     0x1dbe
    29e4:	80 ce 0c             	or     $0xc,%dh
    29e7:	83 c1 01             	add    $0x1,%ecx
    29ea:	83 c0 02             	add    $0x2,%eax
    29ed:	66 89 50 fe          	mov    %dx,-0x2(%eax)
    29f1:	66 0f be 91 38 58 00 	movsbw 0x5838(%ecx),%dx
    29f8:	00 
    29f9:	83 f9 50             	cmp    $0x50,%ecx
    29fc:	75 de                	jne    0x29dc
    29fe:	e9 bb f3 ff ff       	jmp    0x1dbe
    2a03:	a1 34 59 00 00       	mov    0x5934,%eax
    2a08:	bf 30 00 00 00       	mov    $0x30,%edi
    2a0d:	8d 9d e8 fd ff ff    	lea    -0x218(%ebp),%ebx
    2a13:	66 89 bd e8 fd ff ff 	mov    %di,-0x218(%ebp)
    2a1a:	8d 14 80             	lea    (%eax,%eax,4),%edx
    2a1d:	b8 30 00 00 00       	mov    $0x30,%eax
    2a22:	c1 e2 04             	shl    $0x4,%edx
    2a25:	e9 71 f5 ff ff       	jmp    0x1f9b
    2a2a:	52                   	push   %edx
    2a2b:	52                   	push   %edx
    2a2c:	68 a0 6d 00 00       	push   $0x6da0
    2a31:	51                   	push   %ecx
    2a32:	e8 39 07 00 00       	call   0x3170
    2a37:	59                   	pop    %ecx
    2a38:	5b                   	pop    %ebx
    2a39:	68 60 68 00 00       	push   $0x6860
    2a3e:	68 96 00 00 00       	push   $0x96
    2a43:	e8 28 07 00 00       	call   0x3170
    2a48:	83 c4 10             	add    $0x10,%esp
    2a4b:	e9 85 fd ff ff       	jmp    0x27d5
    2a50:	80 7b 06 00          	cmpb   $0x0,0x6(%ebx)
    2a54:	0f 84 47 fd ff ff    	je     0x27a1
    2a5a:	89 8d e4 fd ff ff    	mov    %ecx,-0x21c(%ebp)
    2a60:	31 c0                	xor    %eax,%eax
    2a62:	0f b6 4c 03 06       	movzbl 0x6(%ebx,%eax,1),%ecx
    2a67:	84 c9                	test   %cl,%cl
    2a69:	74 0f                	je     0x2a7a
    2a6b:	88 8c 02 60 68 00 00 	mov    %cl,0x6860(%edx,%eax,1)
    2a72:	83 c0 01             	add    $0x1,%eax
    2a75:	83 f8 0b             	cmp    $0xb,%eax
    2a78:	75 e8                	jne    0x2a62
    2a7a:	6b c6 18             	imul   $0x18,%esi,%eax
    2a7d:	8b 8d e4 fd ff ff    	mov    -0x21c(%ebp),%ecx
    2a83:	c6 80 6b 68 00 00 00 	movb   $0x0,0x686b(%eax)
    2a8a:	e9 39 fd ff ff       	jmp    0x27c8
    2a8f:	e8 0c e7 ff ff       	call   0x11a0
    2a94:	e9 d5 fc ff ff       	jmp    0x276e
    2a99:	e8 02 e7 ff ff       	call   0x11a0
    2a9e:	e9 1f f7 ff ff       	jmp    0x21c2
    2aa3:	3c 77                	cmp    $0x77,%al
    2aa5:	0f 85 cd f2 ff ff    	jne    0x1d78
    2aab:	80 7b 01 69          	cmpb   $0x69,0x1(%ebx)
    2aaf:	0f 85 c3 f2 ff ff    	jne    0x1d78
    2ab5:	80 7b 02 70          	cmpb   $0x70,0x2(%ebx)
    2ab9:	0f 85 b9 f2 ff ff    	jne    0x1d78
    2abf:	80 7b 03 65          	cmpb   $0x65,0x3(%ebx)
    2ac3:	0f 85 af f2 ff ff    	jne    0x1d78
    2ac9:	a1 34 59 00 00       	mov    0x5934,%eax
    2ace:	31 c9                	xor    %ecx,%ecx
    2ad0:	ba 57 00 00 00       	mov    $0x57,%edx
    2ad5:	8d 04 80             	lea    (%eax,%eax,4),%eax
    2ad8:	c1 e0 05             	shl    $0x5,%eax
    2adb:	05 00 80 0b 00       	add    $0xb8000,%eax
    2ae0:	eb 05                	jmp    0x2ae7
    2ae2:	83 f9 50             	cmp    $0x50,%ecx
    2ae5:	74 19                	je     0x2b00
    2ae7:	80 ce 0e             	or     $0xe,%dh
    2aea:	83 c1 01             	add    $0x1,%ecx
    2aed:	83 c0 02             	add    $0x2,%eax
    2af0:	66 89 50 fe          	mov    %dx,-0x2(%eax)
    2af4:	66 0f be 91 a1 57 00 	movsbw 0x57a1(%ecx),%dx
    2afb:	00 
    2afc:	84 d2                	test   %dl,%dl
    2afe:	75 e2                	jne    0x2ae2
    2b00:	a1 34 59 00 00       	mov    0x5934,%eax
    2b05:	83 f8 17             	cmp    $0x17,%eax
    2b08:	0f 8f e4 00 00 00    	jg     0x2bf2
    2b0e:	a1 34 59 00 00       	mov    0x5934,%eax
    2b13:	83 c0 01             	add    $0x1,%eax
    2b16:	a3 34 59 00 00       	mov    %eax,0x5934
    2b1b:	8d 9d e8 fd ff ff    	lea    -0x218(%ebp),%ebx
    2b21:	8d 55 e8             	lea    -0x18(%ebp),%edx
    2b24:	89 d8                	mov    %ebx,%eax
    2b26:	c7 00 00 00 00 00    	movl   $0x0,(%eax)
    2b2c:	83 c0 08             	add    $0x8,%eax
    2b2f:	c7 40 fc 00 00 00 00 	movl   $0x0,-0x4(%eax)
    2b36:	39 c2                	cmp    %eax,%edx
    2b38:	75 ec                	jne    0x2b26
    2b3a:	83 ec 08             	sub    $0x8,%esp
    2b3d:	53                   	push   %ebx
    2b3e:	68 96 00 00 00       	push   $0x96
    2b43:	e8 28 06 00 00       	call   0x3170
    2b48:	b8 60 68 00 00       	mov    $0x6860,%eax
    2b4d:	ba b8 6a 00 00       	mov    $0x6ab8,%edx
    2b52:	83 c4 10             	add    $0x10,%esp
    2b55:	c7 40 14 00 00 00 00 	movl   $0x0,0x14(%eax)
    2b5c:	83 c0 18             	add    $0x18,%eax
    2b5f:	c6 40 e8 00          	movb   $0x0,-0x18(%eax)
    2b63:	c7 40 f4 00 00 00 00 	movl   $0x0,-0xc(%eax)
    2b6a:	c7 40 f8 00 00 00 00 	movl   $0x0,-0x8(%eax)
    2b71:	39 c2                	cmp    %eax,%edx
    2b73:	75 e0                	jne    0x2b55
    2b75:	be c8 00 00 00       	mov    $0xc8,%esi
    2b7a:	83 ec 08             	sub    $0x8,%esp
    2b7d:	53                   	push   %ebx
    2b7e:	56                   	push   %esi
    2b7f:	83 c6 01             	add    $0x1,%esi
    2b82:	e8 e9 05 00 00       	call   0x3170
    2b87:	83 c4 10             	add    $0x10,%esp
    2b8a:	81 fe e1 00 00 00    	cmp    $0xe1,%esi
    2b90:	75 e8                	jne    0x2b7a
    2b92:	31 c0                	xor    %eax,%eax
    2b94:	c7 04 85 a0 6d 00 00 	movl   $0x0,0x6da0(,%eax,4)
    2b9b:	00 00 00 00 
    2b9f:	c7 04 85 a4 6d 00 00 	movl   $0x0,0x6da4(,%eax,4)
    2ba6:	00 00 00 00 
    2baa:	83 c0 02             	add    $0x2,%eax
    2bad:	83 f8 10             	cmp    $0x10,%eax
    2bb0:	75 e2                	jne    0x2b94
    2bb2:	a1 34 59 00 00       	mov    0x5934,%eax
    2bb7:	31 c9                	xor    %ecx,%ecx
    2bb9:	ba 44 00 00 00       	mov    $0x44,%edx
    2bbe:	8d 04 80             	lea    (%eax,%eax,4),%eax
    2bc1:	c1 e0 05             	shl    $0x5,%eax
    2bc4:	05 00 80 0b 00       	add    $0xb8000,%eax
    2bc9:	eb 08                	jmp    0x2bd3
    2bcb:	84 d2                	test   %dl,%dl
    2bcd:	0f 84 eb f1 ff ff    	je     0x1dbe
    2bd3:	80 ce 0a             	or     $0xa,%dh
    2bd6:	83 c1 01             	add    $0x1,%ecx
    2bd9:	83 c0 02             	add    $0x2,%eax
    2bdc:	66 89 50 fe          	mov    %dx,-0x2(%eax)
    2be0:	66 0f be 91 bb 57 00 	movsbw 0x57bb(%ecx),%dx
    2be7:	00 
    2be8:	83 f9 50             	cmp    $0x50,%ecx
    2beb:	75 de                	jne    0x2bcb
    2bed:	e9 cc f1 ff ff       	jmp    0x1dbe
    2bf2:	e8 a9 e5 ff ff       	call   0x11a0
    2bf7:	e9 1f ff ff ff       	jmp    0x2b1b
    2bfc:	89 da                	mov    %ebx,%edx
    2bfe:	e9 7d fa ff ff       	jmp    0x2680
    2c03:	89 da                	mov    %ebx,%edx
    2c05:	8b 5d 08             	mov    0x8(%ebp),%ebx
    2c08:	e9 fb f9 ff ff       	jmp    0x2608
    2c0d:	6b da 18             	imul   $0x18,%edx,%ebx
    2c10:	50                   	push   %eax
    2c11:	50                   	push   %eax
    2c12:	68 a0 6d 00 00       	push   $0x6da0
    2c17:	ff b3 6c 68 00 00    	push   0x686c(%ebx)
    2c1d:	81 c3 60 68 00 00    	add    $0x6860,%ebx
    2c23:	e8 18 06 00 00       	call   0x3240
    2c28:	a1 34 59 00 00       	mov    0x5934,%eax
    2c2d:	6a 0a                	push   $0xa
    2c2f:	6a 00                	push   $0x0
    2c31:	50                   	push   %eax
    2c32:	68 84 57 00 00       	push   $0x5784
    2c37:	e8 54 ee ff ff       	call   0x1a90
    2c3c:	a1 34 59 00 00       	mov    0x5934,%eax
    2c41:	83 c4 20             	add    $0x20,%esp
    2c44:	6a 0f                	push   $0xf
    2c46:	6a 08                	push   $0x8
    2c48:	50                   	push   %eax
    2c49:	53                   	push   %ebx
    2c4a:	e8 41 ee ff ff       	call   0x1a90
    2c4f:	83 c4 10             	add    $0x10,%esp
    2c52:	e9 67 f1 ff ff       	jmp    0x1dbe
    2c57:	e8 44 e5 ff ff       	call   0x11a0
    2c5c:	e9 7b f4 ff ff       	jmp    0x20dc
    2c61:	a1 34 59 00 00       	mov    0x5934,%eax
    2c66:	31 c9                	xor    %ecx,%ecx
    2c68:	ba 45 00 00 00       	mov    $0x45,%edx
    2c6d:	8d 04 80             	lea    (%eax,%eax,4),%eax
    2c70:	c1 e0 05             	shl    $0x5,%eax
    2c73:	05 00 80 0b 00       	add    $0xb8000,%eax
    2c78:	eb 09                	jmp    0x2c83
    2c7a:	83 f9 50             	cmp    $0x50,%ecx
    2c7d:	0f 84 3b f1 ff ff    	je     0x1dbe
    2c83:	80 ce 0c             	or     $0xc,%dh
    2c86:	83 c1 01             	add    $0x1,%ecx
    2c89:	83 c0 02             	add    $0x2,%eax
    2c8c:	66 89 50 fe          	mov    %dx,-0x2(%eax)
    2c90:	66 0f be 91 f7 56 00 	movsbw 0x56f7(%ecx),%dx
    2c97:	00 
    2c98:	84 d2                	test   %dl,%dl
    2c9a:	75 de                	jne    0x2c7a
    2c9c:	e9 1d f1 ff ff       	jmp    0x1dbe
    2ca1:	e8 fa e4 ff ff       	call   0x11a0
    2ca6:	e9 df f3 ff ff       	jmp    0x208a
    2cab:	89 c2                	mov    %eax,%edx
    2cad:	be 21 2f 00 00       	mov    $0x2f21,%esi
    2cb2:	e8 59 d7 ff ff       	call   0x410
    2cb7:	b8 53 2f 00 00       	mov    $0x2f53,%eax
    2cbc:	ba 45 2f 00 00       	mov    $0x2f45,%edx
    2cc1:	b9 44 2f 00 00       	mov    $0x2f44,%ecx
    2cc6:	66 89 87 00 80 0b 00 	mov    %ax,0xb8000(%edi)
    2ccd:	b8 41 2f 00 00       	mov    $0x2f41,%eax
    2cd2:	bf 4f 2f 00 00       	mov    $0x2f4f,%edi
    2cd7:	66 89 83 00 80 0b 00 	mov    %ax,0xb8000(%ebx)
    2cde:	b8 56 2f 00 00       	mov    $0x2f56,%eax
    2ce3:	66 89 83 02 80 0b 00 	mov    %ax,0xb8002(%ebx)
    2cea:	b8 4b 2f 00 00       	mov    $0x2f4b,%eax
    2cef:	66 89 93 04 80 0b 00 	mov    %dx,0xb8004(%ebx)
    2cf6:	66 89 8b 06 80 0b 00 	mov    %cx,0xb8006(%ebx)
    2cfd:	66 89 b3 08 80 0b 00 	mov    %si,0xb8008(%ebx)
    2d04:	66 89 3d 12 82 0b 00 	mov    %di,0xb8212
    2d0b:	66 a3 14 82 0b 00    	mov    %ax,0xb8214
    2d11:	e9 a8 f0 ff ff       	jmp    0x1dbe
    2d16:	c7 87 00 80 0b 00 53 	movl   $0x4f4c4f53,0xb8000(%edi)
    2d1d:	4f 4c 4f 
    2d20:	ba 45 4f 00 00       	mov    $0x4f45,%edx
    2d25:	c7 40 04 4f 4f 54 4f 	movl   $0x4f544f4f,0x4(%eax)
    2d2c:	c7 40 08 20 4f 45 4f 	movl   $0x4f454f20,0x8(%eax)
    2d33:	c7 40 0c 4d 4f 50 4f 	movl   $0x4f504f4d,0xc(%eax)
    2d3a:	c7 40 10 54 4f 59 4f 	movl   $0x4f594f54,0x10(%eax)
    2d41:	b8 52 4f 00 00       	mov    $0x4f52,%eax
    2d46:	e9 39 f8 ff ff       	jmp    0x2584
    2d4b:	ba 45 00 00 00       	mov    $0x45,%edx
    2d50:	eb 09                	jmp    0x2d5b
    2d52:	83 f9 50             	cmp    $0x50,%ecx
    2d55:	0f 84 63 f0 ff ff    	je     0x1dbe
    2d5b:	80 ce 0c             	or     $0xc,%dh
    2d5e:	83 c1 01             	add    $0x1,%ecx
    2d61:	83 c0 02             	add    $0x2,%eax
    2d64:	66 89 50 fe          	mov    %dx,-0x2(%eax)
    2d68:	66 0f be 91 57 57 00 	movsbw 0x5757(%ecx),%dx
    2d6f:	00 
    2d70:	84 d2                	test   %dl,%dl
    2d72:	75 de                	jne    0x2d52
    2d74:	e9 45 f0 ff ff       	jmp    0x1dbe
    2d79:	83 ec 08             	sub    $0x8,%esp
    2d7c:	68 60 68 00 00       	push   $0x6860
    2d81:	68 96 00 00 00       	push   $0x96
    2d86:	e8 b5 04 00 00       	call   0x3240
    2d8b:	83 c4 10             	add    $0x10,%esp
    2d8e:	e9 6e f9 ff ff       	jmp    0x2701
    2d93:	50                   	push   %eax
    2d94:	50                   	push   %eax
    2d95:	68 60 68 00 00       	push   $0x6860
    2d9a:	68 96 00 00 00       	push   $0x96
    2d9f:	e8 9c 04 00 00       	call   0x3240
    2da4:	83 c4 10             	add    $0x10,%esp
    2da7:	e9 4f f8 ff ff       	jmp    0x25fb
    2dac:	a1 34 59 00 00       	mov    0x5934,%eax
    2db1:	31 c9                	xor    %ecx,%ecx
    2db3:	ba 55 00 00 00       	mov    $0x55,%edx
    2db8:	8d 04 80             	lea    (%eax,%eax,4),%eax
    2dbb:	c1 e0 05             	shl    $0x5,%eax
    2dbe:	05 00 80 0b 00       	add    $0xb8000,%eax
    2dc3:	eb 08                	jmp    0x2dcd
    2dc5:	84 d2                	test   %dl,%dl
    2dc7:	0f 84 f1 ef ff ff    	je     0x1dbe
    2dcd:	80 ce 0c             	or     $0xc,%dh
    2dd0:	83 c1 01             	add    $0x1,%ecx
    2dd3:	83 c0 02             	add    $0x2,%eax
    2dd6:	66 89 50 fe          	mov    %dx,-0x2(%eax)
    2dda:	66 0f be 91 70 57 00 	movsbw 0x5770(%ecx),%dx
    2de1:	00 
    2de2:	83 f9 50             	cmp    $0x50,%ecx
    2de5:	75 de                	jne    0x2dc5
    2de7:	e9 d2 ef ff ff       	jmp    0x1dbe
    2dec:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    2df0:	e4 60                	in     $0x60,%al
    2df2:	84 c0                	test   %al,%al
    2df4:	78 3a                	js     0x2e30
    2df6:	55                   	push   %ebp
    2df7:	0f b6 d0             	movzbl %al,%edx
    2dfa:	89 e5                	mov    %esp,%ebp
    2dfc:	83 ec 28             	sub    $0x28,%esp
    2dff:	66 0f be 92 60 54 00 	movsbw 0x5460(%edx),%dx
    2e06:	00 
    2e07:	3c 1c                	cmp    $0x1c,%al
    2e09:	74 2d                	je     0x2e38
    2e0b:	3c 0e                	cmp    $0xe,%al
    2e0d:	0f 84 4d 01 00 00    	je     0x2f60
    2e13:	83 e8 3b             	sub    $0x3b,%eax
    2e16:	3c 04                	cmp    $0x4,%al
    2e18:	0f 87 4f 01 00 00    	ja     0x2f6d
    2e1e:	0f b6 c0             	movzbl %al,%eax
    2e21:	ff 24 85 40 54 00 00 	jmp    *0x5440(,%eax,4)
    2e28:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    2e2f:	90                   	nop
    2e30:	c3                   	ret
    2e31:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    2e38:	a1 20 68 00 00       	mov    0x6820,%eax
    2e3d:	8d 4d d8             	lea    -0x28(%ebp),%ecx
    2e40:	c6 80 40 68 00 00 00 	movb   $0x0,0x6840(%eax)
    2e47:	a1 20 68 00 00       	mov    0x6820,%eax
    2e4c:	85 c0                	test   %eax,%eax
    2e4e:	78 2c                	js     0x2e7c
    2e50:	31 c0                	xor    %eax,%eax
    2e52:	8d 4d d8             	lea    -0x28(%ebp),%ecx
    2e55:	eb 0e                	jmp    0x2e65
    2e57:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    2e5e:	66 90                	xchg   %ax,%ax
    2e60:	83 f8 20             	cmp    $0x20,%eax
    2e63:	74 17                	je     0x2e7c
    2e65:	0f b6 90 40 68 00 00 	movzbl 0x6840(%eax),%edx
    2e6c:	88 14 01             	mov    %dl,(%ecx,%eax,1)
    2e6f:	8b 15 20 68 00 00    	mov    0x6820,%edx
    2e75:	83 c0 01             	add    $0x1,%eax
    2e78:	39 c2                	cmp    %eax,%edx
    2e7a:	7d e4                	jge    0x2e60
    2e7c:	83 ec 0c             	sub    $0xc,%esp
    2e7f:	c6 45 f7 00          	movb   $0x0,-0x9(%ebp)
    2e83:	51                   	push   %ecx
    2e84:	e8 07 ee ff ff       	call   0x1c90
    2e89:	b8 4b 0f 00 00       	mov    $0xf4b,%eax
    2e8e:	83 c4 10             	add    $0x10,%esp
    2e91:	c7 05 20 68 00 00 00 	movl   $0x0,0x6820
    2e98:	00 00 00 
    2e9b:	66 a3 20 83 0b 00    	mov    %ax,0xb8320
    2ea1:	c9                   	leave
    2ea2:	c3                   	ret
    2ea3:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    2ea7:	90                   	nop
    2ea8:	31 c0                	xor    %eax,%eax
    2eaa:	31 d2                	xor    %edx,%edx
    2eac:	e8 5f d5 ff ff       	call   0x410
    2eb1:	b8 57 2f 00 00       	mov    $0x2f57,%eax
    2eb6:	66 a3 9a 80 0b 00    	mov    %ax,0xb809a
    2ebc:	b9 4b 0f 00 00       	mov    $0xf4b,%ecx
    2ec1:	66 89 0d 20 83 0b 00 	mov    %cx,0xb8320
    2ec8:	c9                   	leave
    2ec9:	c3                   	ret
    2eca:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    2ed0:	e8 eb df ff ff       	call   0xec0
    2ed5:	b8 4b 0f 00 00       	mov    $0xf4b,%eax
    2eda:	66 a3 20 83 0b 00    	mov    %ax,0xb8320
    2ee0:	c9                   	leave
    2ee1:	c3                   	ret
    2ee2:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    2ee8:	83 ec 08             	sub    $0x8,%esp
    2eeb:	6a 00                	push   $0x0
    2eed:	6a 00                	push   $0x0
    2eef:	e8 5c e1 ff ff       	call   0x1050
    2ef4:	b8 4c 1f 00 00       	mov    $0x1f4c,%eax
    2ef9:	ba 4b 0f 00 00       	mov    $0xf4b,%edx
    2efe:	83 c4 10             	add    $0x10,%esp
    2f01:	66 a3 9a 80 0b 00    	mov    %ax,0xb809a
    2f07:	66 89 15 20 83 0b 00 	mov    %dx,0xb8320
    2f0e:	c9                   	leave
    2f0f:	c3                   	ret
    2f10:	31 c0                	xor    %eax,%eax
    2f12:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    2f18:	81 80 20 6d 00 00 e8 	addl   $0x3e8,0x6d20(%eax)
    2f1f:	03 00 00 
    2f22:	83 c0 18             	add    $0x18,%eax
    2f25:	83 f8 78             	cmp    $0x78,%eax
    2f28:	75 ee                	jne    0x2f18
    2f2a:	eb 90                	jmp    0x2ebc
    2f2c:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    2f30:	81 05 a0 6c 00 00 2c 	addl   $0x12c,0x6ca0
    2f37:	01 00 00 
    2f3a:	b8 b8 6c 00 00       	mov    $0x6cb8,%eax
    2f3f:	81 00 2c 01 00 00    	addl   $0x12c,(%eax)
    2f45:	81 40 18 2c 01 00 00 	addl   $0x12c,0x18(%eax)
    2f4c:	83 c0 30             	add    $0x30,%eax
    2f4f:	3d 18 6d 00 00       	cmp    $0x6d18,%eax
    2f54:	75 e9                	jne    0x2f3f
    2f56:	e9 61 ff ff ff       	jmp    0x2ebc
    2f5b:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    2f5f:	90                   	nop
    2f60:	a1 20 68 00 00       	mov    0x6820,%eax
    2f65:	85 c0                	test   %eax,%eax
    2f67:	0f 8f a3 00 00 00    	jg     0x3010
    2f6d:	84 d2                	test   %dl,%dl
    2f6f:	0f 84 db 00 00 00    	je     0x3050
    2f75:	a1 20 68 00 00       	mov    0x6820,%eax
    2f7a:	83 f8 1e             	cmp    $0x1e,%eax
    2f7d:	0f 8f 39 ff ff ff    	jg     0x2ebc
    2f83:	a1 20 68 00 00       	mov    0x6820,%eax
    2f88:	8d 48 01             	lea    0x1(%eax),%ecx
    2f8b:	89 0d 20 68 00 00    	mov    %ecx,0x6820
    2f91:	88 90 40 68 00 00    	mov    %dl,0x6840(%eax)
    2f97:	a1 ec 6d 00 00       	mov    0x6dec,%eax
    2f9c:	83 c0 01             	add    $0x1,%eax
    2f9f:	89 c1                	mov    %eax,%ecx
    2fa1:	c1 f9 1f             	sar    $0x1f,%ecx
    2fa4:	c1 e9 1b             	shr    $0x1b,%ecx
    2fa7:	01 c8                	add    %ecx,%eax
    2fa9:	83 e0 1f             	and    $0x1f,%eax
    2fac:	29 c8                	sub    %ecx,%eax
    2fae:	8b 0d e8 6d 00 00    	mov    0x6de8,%ecx
    2fb4:	39 c1                	cmp    %eax,%ecx
    2fb6:	74 11                	je     0x2fc9
    2fb8:	8b 0d ec 6d 00 00    	mov    0x6dec,%ecx
    2fbe:	88 91 00 6e 00 00    	mov    %dl,0x6e00(%ecx)
    2fc4:	a3 ec 6d 00 00       	mov    %eax,0x6dec
    2fc9:	a1 34 59 00 00       	mov    0x5934,%eax
    2fce:	83 f8 0e             	cmp    $0xe,%eax
    2fd1:	0f 8e e5 fe ff ff    	jle    0x2ebc
    2fd7:	a1 34 59 00 00       	mov    0x5934,%eax
    2fdc:	83 f8 18             	cmp    $0x18,%eax
    2fdf:	0f 8f d7 fe ff ff    	jg     0x2ebc
    2fe5:	a1 34 59 00 00       	mov    0x5934,%eax
    2fea:	8b 0d 20 68 00 00    	mov    0x6820,%ecx
    2ff0:	80 ce 0f             	or     $0xf,%dh
    2ff3:	8d 04 80             	lea    (%eax,%eax,4),%eax
    2ff6:	c1 e0 04             	shl    $0x4,%eax
    2ff9:	8d 84 01 ff ff ff 7f 	lea    0x7fffffff(%ecx,%eax,1),%eax
    3000:	66 89 94 00 00 80 0b 	mov    %dx,0xb8000(%eax,%eax,1)
    3007:	00 
    3008:	e9 af fe ff ff       	jmp    0x2ebc
    300d:	8d 76 00             	lea    0x0(%esi),%esi
    3010:	a1 20 68 00 00       	mov    0x6820,%eax
    3015:	b9 4b 0f 00 00       	mov    $0xf4b,%ecx
    301a:	83 e8 01             	sub    $0x1,%eax
    301d:	a3 20 68 00 00       	mov    %eax,0x6820
    3022:	a1 34 59 00 00       	mov    0x5934,%eax
    3027:	8b 15 20 68 00 00    	mov    0x6820,%edx
    302d:	8d 04 80             	lea    (%eax,%eax,4),%eax
    3030:	c1 e0 04             	shl    $0x4,%eax
    3033:	01 d0                	add    %edx,%eax
    3035:	ba 20 0f 00 00       	mov    $0xf20,%edx
    303a:	66 89 94 00 00 80 0b 	mov    %dx,0xb8000(%eax,%eax,1)
    3041:	00 
    3042:	66 89 0d 20 83 0b 00 	mov    %cx,0xb8320
    3049:	c9                   	leave
    304a:	c3                   	ret
    304b:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    304f:	90                   	nop
    3050:	81 05 a0 6c 00 00 f4 	addl   $0x1f4,0x6ca0
    3057:	01 00 00 
    305a:	e9 5d fe ff ff       	jmp    0x2ebc
    305f:	90                   	nop
    3060:	55                   	push   %ebp
    3061:	b8 cd cc cc cc       	mov    $0xcccccccd,%eax
    3066:	89 e5                	mov    %esp,%ebp
    3068:	57                   	push   %edi
    3069:	56                   	push   %esi
    306a:	53                   	push   %ebx
    306b:	83 ec 1c             	sub    $0x1c,%esp
    306e:	8b 5d 08             	mov    0x8(%ebp),%ebx
    3071:	8b 4d 0c             	mov    0xc(%ebp),%ecx
    3074:	f7 e1                	mul    %ecx
    3076:	8d 04 9b             	lea    (%ebx,%ebx,4),%eax
    3079:	c1 e0 03             	shl    $0x3,%eax
    307c:	8d 58 64             	lea    0x64(%eax),%ebx
    307f:	05 82 00 00 00       	add    $0x82,%eax
    3084:	89 45 e4             	mov    %eax,-0x1c(%ebp)
    3087:	89 d6                	mov    %edx,%esi
    3089:	c1 ee 03             	shr    $0x3,%esi
    308c:	83 f9 09             	cmp    $0x9,%ecx
    308f:	76 41                	jbe    0x30d2
    3091:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    3098:	31 ff                	xor    %edi,%edi
    309a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    30a0:	ba f4 01 00 00       	mov    $0x1f4,%edx
    30a5:	83 ec 04             	sub    $0x4,%esp
    30a8:	29 fa                	sub    %edi,%edx
    30aa:	68 00 ff 00 00       	push   $0xff00
    30af:	83 c7 01             	add    $0x1,%edi
    30b2:	52                   	push   %edx
    30b3:	53                   	push   %ebx
    30b4:	e8 47 0a 00 00       	call   0x3b00
    30b9:	83 c4 10             	add    $0x10,%esp
    30bc:	39 f7                	cmp    %esi,%edi
    30be:	72 e0                	jb     0x30a0
    30c0:	8b 45 e4             	mov    -0x1c(%ebp),%eax
    30c3:	83 c3 01             	add    $0x1,%ebx
    30c6:	39 c3                	cmp    %eax,%ebx
    30c8:	75 ce                	jne    0x3098
    30ca:	8d 65 f4             	lea    -0xc(%ebp),%esp
    30cd:	5b                   	pop    %ebx
    30ce:	5e                   	pop    %esi
    30cf:	5f                   	pop    %edi
    30d0:	5d                   	pop    %ebp
    30d1:	c3                   	ret
    30d2:	8b 45 e4             	mov    -0x1c(%ebp),%eax
    30d5:	83 c3 01             	add    $0x1,%ebx
    30d8:	39 c3                	cmp    %eax,%ebx
    30da:	75 b0                	jne    0x308c
    30dc:	8d 65 f4             	lea    -0xc(%ebp),%esp
    30df:	5b                   	pop    %ebx
    30e0:	5e                   	pop    %esi
    30e1:	5f                   	pop    %edi
    30e2:	5d                   	pop    %ebp
    30e3:	c3                   	ret
    30e4:	66 90                	xchg   %ax,%ax
    30e6:	66 90                	xchg   %ax,%ax
    30e8:	66 90                	xchg   %ax,%ax
    30ea:	66 90                	xchg   %ax,%ax
    30ec:	66 90                	xchg   %ax,%ax
    30ee:	66 90                	xchg   %ax,%ax
    30f0:	b8 02 00 00 00       	mov    $0x2,%eax
    30f5:	ba f6 03 00 00       	mov    $0x3f6,%edx
    30fa:	ee                   	out    %al,(%dx)
    30fb:	ba f7 01 00 00       	mov    $0x1f7,%edx
    3100:	ec                   	in     (%dx),%al
    3101:	31 c9                	xor    %ecx,%ecx
    3103:	3c ff                	cmp    $0xff,%al
    3105:	74 2d                	je     0x3134
    3107:	ec                   	in     (%dx),%al
    3108:	84 c0                	test   %al,%al
    310a:	79 16                	jns    0x3122
    310c:	b9 40 0d 03 00       	mov    $0x30d40,%ecx
    3111:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    3118:	83 e9 01             	sub    $0x1,%ecx
    311b:	74 2e                	je     0x314b
    311d:	ec                   	in     (%dx),%al
    311e:	84 c0                	test   %al,%al
    3120:	78 f6                	js     0x3118
    3122:	ba f3 01 00 00       	mov    $0x1f3,%edx
    3127:	b8 ab ff ff ff       	mov    $0xffffffab,%eax
    312c:	ee                   	out    %al,(%dx)
    312d:	ec                   	in     (%dx),%al
    312e:	31 c9                	xor    %ecx,%ecx
    3130:	3c ab                	cmp    $0xab,%al
    3132:	74 0c                	je     0x3140
    3134:	89 c8                	mov    %ecx,%eax
    3136:	c3                   	ret
    3137:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    313e:	66 90                	xchg   %ax,%ax
    3140:	31 c0                	xor    %eax,%eax
    3142:	ee                   	out    %al,(%dx)
    3143:	b9 01 00 00 00       	mov    $0x1,%ecx
    3148:	89 c8                	mov    %ecx,%eax
    314a:	c3                   	ret
    314b:	31 c9                	xor    %ecx,%ecx
    314d:	89 c8                	mov    %ecx,%eax
    314f:	c3                   	ret
    3150:	ba f7 01 00 00       	mov    $0x1f7,%edx
    3155:	ec                   	in     (%dx),%al
    3156:	b9 1f a1 07 00       	mov    $0x7a11f,%ecx
    315b:	eb 09                	jmp    0x3166
    315d:	8d 76 00             	lea    0x0(%esi),%esi
    3160:	ec                   	in     (%dx),%al
    3161:	83 e9 01             	sub    $0x1,%ecx
    3164:	74 08                	je     0x316e
    3166:	84 c0                	test   %al,%al
    3168:	78 f6                	js     0x3160
    316a:	a8 40                	test   $0x40,%al
    316c:	74 f2                	je     0x3160
    316e:	c3                   	ret
    316f:	90                   	nop
    3170:	55                   	push   %ebp
    3171:	ba f7 01 00 00       	mov    $0x1f7,%edx
    3176:	89 e5                	mov    %esp,%ebp
    3178:	53                   	push   %ebx
    3179:	8b 4d 08             	mov    0x8(%ebp),%ecx
    317c:	ec                   	in     (%dx),%al
    317d:	bb 1f a1 07 00       	mov    $0x7a11f,%ebx
    3182:	eb 0a                	jmp    0x318e
    3184:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    3188:	ec                   	in     (%dx),%al
    3189:	83 eb 01             	sub    $0x1,%ebx
    318c:	74 08                	je     0x3196
    318e:	84 c0                	test   %al,%al
    3190:	78 f6                	js     0x3188
    3192:	a8 40                	test   $0x40,%al
    3194:	74 f2                	je     0x3188
    3196:	89 c8                	mov    %ecx,%eax
    3198:	ba f6 01 00 00       	mov    $0x1f6,%edx
    319d:	c1 e8 18             	shr    $0x18,%eax
    31a0:	83 e0 0f             	and    $0xf,%eax
    31a3:	83 c8 e0             	or     $0xffffffe0,%eax
    31a6:	ee                   	out    %al,(%dx)
    31a7:	b8 01 00 00 00       	mov    $0x1,%eax
    31ac:	ba f2 01 00 00       	mov    $0x1f2,%edx
    31b1:	ee                   	out    %al,(%dx)
    31b2:	ba f3 01 00 00       	mov    $0x1f3,%edx
    31b7:	89 c8                	mov    %ecx,%eax
    31b9:	ee                   	out    %al,(%dx)
    31ba:	89 c8                	mov    %ecx,%eax
    31bc:	ba f4 01 00 00       	mov    $0x1f4,%edx
    31c1:	c1 e8 08             	shr    $0x8,%eax
    31c4:	ee                   	out    %al,(%dx)
    31c5:	89 c8                	mov    %ecx,%eax
    31c7:	ba f5 01 00 00       	mov    $0x1f5,%edx
    31cc:	c1 e8 10             	shr    $0x10,%eax
    31cf:	ee                   	out    %al,(%dx)
    31d0:	b8 30 00 00 00       	mov    $0x30,%eax
    31d5:	ba f7 01 00 00       	mov    $0x1f7,%edx
    31da:	ee                   	out    %al,(%dx)
    31db:	b9 20 a1 07 00       	mov    $0x7a120,%ecx
    31e0:	ec                   	in     (%dx),%al
    31e1:	83 e9 01             	sub    $0x1,%ecx
    31e4:	74 04                	je     0x31ea
    31e6:	a8 08                	test   $0x8,%al
    31e8:	74 f6                	je     0x31e0
    31ea:	8b 4d 0c             	mov    0xc(%ebp),%ecx
    31ed:	ba f0 01 00 00       	mov    $0x1f0,%edx
    31f2:	8d 99 00 02 00 00    	lea    0x200(%ecx),%ebx
    31f8:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    31ff:	90                   	nop
    3200:	0f b7 01             	movzwl (%ecx),%eax
    3203:	66 ef                	out    %ax,(%dx)
    3205:	83 c1 02             	add    $0x2,%ecx
    3208:	39 d9                	cmp    %ebx,%ecx
    320a:	75 f4                	jne    0x3200
    320c:	ba f7 01 00 00       	mov    $0x1f7,%edx
    3211:	b8 e7 ff ff ff       	mov    $0xffffffe7,%eax
    3216:	ee                   	out    %al,(%dx)
    3217:	ec                   	in     (%dx),%al
    3218:	b9 1f a1 07 00       	mov    $0x7a11f,%ecx
    321d:	eb 07                	jmp    0x3226
    321f:	90                   	nop
    3220:	ec                   	in     (%dx),%al
    3221:	83 e9 01             	sub    $0x1,%ecx
    3224:	74 08                	je     0x322e
    3226:	84 c0                	test   %al,%al
    3228:	78 f6                	js     0x3220
    322a:	a8 40                	test   $0x40,%al
    322c:	74 f2                	je     0x3220
    322e:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    3231:	c9                   	leave
    3232:	c3                   	ret
    3233:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    323a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    3240:	55                   	push   %ebp
    3241:	ba f7 01 00 00       	mov    $0x1f7,%edx
    3246:	89 e5                	mov    %esp,%ebp
    3248:	53                   	push   %ebx
    3249:	8b 4d 08             	mov    0x8(%ebp),%ecx
    324c:	ec                   	in     (%dx),%al
    324d:	bb 1f a1 07 00       	mov    $0x7a11f,%ebx
    3252:	eb 0a                	jmp    0x325e
    3254:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    3258:	ec                   	in     (%dx),%al
    3259:	83 eb 01             	sub    $0x1,%ebx
    325c:	74 08                	je     0x3266
    325e:	84 c0                	test   %al,%al
    3260:	78 f6                	js     0x3258
    3262:	a8 40                	test   $0x40,%al
    3264:	74 f2                	je     0x3258
    3266:	89 c8                	mov    %ecx,%eax
    3268:	ba f6 01 00 00       	mov    $0x1f6,%edx
    326d:	c1 e8 18             	shr    $0x18,%eax
    3270:	83 e0 0f             	and    $0xf,%eax
    3273:	83 c8 e0             	or     $0xffffffe0,%eax
    3276:	ee                   	out    %al,(%dx)
    3277:	b8 01 00 00 00       	mov    $0x1,%eax
    327c:	ba f2 01 00 00       	mov    $0x1f2,%edx
    3281:	ee                   	out    %al,(%dx)
    3282:	ba f3 01 00 00       	mov    $0x1f3,%edx
    3287:	89 c8                	mov    %ecx,%eax
    3289:	ee                   	out    %al,(%dx)
    328a:	89 c8                	mov    %ecx,%eax
    328c:	ba f4 01 00 00       	mov    $0x1f4,%edx
    3291:	c1 e8 08             	shr    $0x8,%eax
    3294:	ee                   	out    %al,(%dx)
    3295:	89 c8                	mov    %ecx,%eax
    3297:	ba f5 01 00 00       	mov    $0x1f5,%edx
    329c:	c1 e8 10             	shr    $0x10,%eax
    329f:	ee                   	out    %al,(%dx)
    32a0:	b8 20 00 00 00       	mov    $0x20,%eax
    32a5:	ba f7 01 00 00       	mov    $0x1f7,%edx
    32aa:	ee                   	out    %al,(%dx)
    32ab:	b9 20 a1 07 00       	mov    $0x7a120,%ecx
    32b0:	ec                   	in     (%dx),%al
    32b1:	83 e9 01             	sub    $0x1,%ecx
    32b4:	74 04                	je     0x32ba
    32b6:	a8 08                	test   $0x8,%al
    32b8:	74 f6                	je     0x32b0
    32ba:	8b 4d 0c             	mov    0xc(%ebp),%ecx
    32bd:	ba f0 01 00 00       	mov    $0x1f0,%edx
    32c2:	8d 99 00 02 00 00    	lea    0x200(%ecx),%ebx
    32c8:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    32cf:	90                   	nop
    32d0:	66 ed                	in     (%dx),%ax
    32d2:	66 89 01             	mov    %ax,(%ecx)
    32d5:	83 c1 02             	add    $0x2,%ecx
    32d8:	39 cb                	cmp    %ecx,%ebx
    32da:	75 f4                	jne    0x32d0
    32dc:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    32df:	c9                   	leave
    32e0:	c3                   	ret
    32e1:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    32e8:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    32ef:	90                   	nop
    32f0:	55                   	push   %ebp
    32f1:	31 d2                	xor    %edx,%edx
    32f3:	89 e5                	mov    %esp,%ebp
    32f5:	8b 45 08             	mov    0x8(%ebp),%eax
    32f8:	83 c0 14             	add    $0x14,%eax
    32fb:	eb 0e                	jmp    0x330b
    32fd:	8d 76 00             	lea    0x0(%esi),%esi
    3300:	83 c2 01             	add    $0x1,%edx
    3303:	83 c0 18             	add    $0x18,%eax
    3306:	83 fa 19             	cmp    $0x19,%edx
    3309:	74 0d                	je     0x3318
    330b:	8b 08                	mov    (%eax),%ecx
    330d:	85 c9                	test   %ecx,%ecx
    330f:	75 ef                	jne    0x3300
    3311:	89 d0                	mov    %edx,%eax
    3313:	5d                   	pop    %ebp
    3314:	c3                   	ret
    3315:	8d 76 00             	lea    0x0(%esi),%esi
    3318:	ba ff ff ff ff       	mov    $0xffffffff,%edx
    331d:	5d                   	pop    %ebp
    331e:	89 d0                	mov    %edx,%eax
    3320:	c3                   	ret
    3321:	66 90                	xchg   %ax,%ax
    3323:	66 90                	xchg   %ax,%ax
    3325:	66 90                	xchg   %ax,%ax
    3327:	66 90                	xchg   %ax,%ax
    3329:	66 90                	xchg   %ax,%ax
    332b:	66 90                	xchg   %ax,%ax
    332d:	66 90                	xchg   %ax,%ax
    332f:	90                   	nop
    3330:	55                   	push   %ebp
    3331:	89 e5                	mov    %esp,%ebp
    3333:	57                   	push   %edi
    3334:	89 d7                	mov    %edx,%edi
    3336:	56                   	push   %esi
    3337:	53                   	push   %ebx
    3338:	83 ec 14             	sub    $0x14,%esp
    333b:	85 c0                	test   %eax,%eax
    333d:	7e 60                	jle    0x339f
    333f:	89 55 e0             	mov    %edx,-0x20(%ebp)
    3342:	89 c3                	mov    %eax,%ebx
    3344:	31 f6                	xor    %esi,%esi
    3346:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    334d:	8d 76 00             	lea    0x0(%esi),%esi
    3350:	b8 cd cc cc cc       	mov    $0xcccccccd,%eax
    3355:	89 f1                	mov    %esi,%ecx
    3357:	83 c6 01             	add    $0x1,%esi
    335a:	f7 e3                	mul    %ebx
    335c:	89 d8                	mov    %ebx,%eax
    335e:	c1 ea 03             	shr    $0x3,%edx
    3361:	8d 3c 92             	lea    (%edx,%edx,4),%edi
    3364:	01 ff                	add    %edi,%edi
    3366:	29 f8                	sub    %edi,%eax
    3368:	83 c0 30             	add    $0x30,%eax
    336b:	88 44 35 e7          	mov    %al,-0x19(%ebp,%esi,1)
    336f:	89 d8                	mov    %ebx,%eax
    3371:	89 d3                	mov    %edx,%ebx
    3373:	83 f8 09             	cmp    $0x9,%eax
    3376:	7f d8                	jg     0x3350
    3378:	8b 7d e0             	mov    -0x20(%ebp),%edi
    337b:	8d 45 e8             	lea    -0x18(%ebp),%eax
    337e:	01 c1                	add    %eax,%ecx
    3380:	89 fa                	mov    %edi,%edx
    3382:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    3388:	0f b6 01             	movzbl (%ecx),%eax
    338b:	8d 5d e8             	lea    -0x18(%ebp),%ebx
    338e:	83 c2 01             	add    $0x1,%edx
    3391:	88 42 ff             	mov    %al,-0x1(%edx)
    3394:	89 c8                	mov    %ecx,%eax
    3396:	83 e9 01             	sub    $0x1,%ecx
    3399:	39 c3                	cmp    %eax,%ebx
    339b:	75 eb                	jne    0x3388
    339d:	01 f7                	add    %esi,%edi
    339f:	c6 07 00             	movb   $0x0,(%edi)
    33a2:	83 c4 14             	add    $0x14,%esp
    33a5:	5b                   	pop    %ebx
    33a6:	5e                   	pop    %esi
    33a7:	5f                   	pop    %edi
    33a8:	5d                   	pop    %ebp
    33a9:	c3                   	ret
    33aa:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    33b0:	55                   	push   %ebp
    33b1:	b8 01 00 00 00       	mov    $0x1,%eax
    33b6:	89 e5                	mov    %esp,%ebp
    33b8:	8b 4d 08             	mov    0x8(%ebp),%ecx
    33bb:	5d                   	pop    %ebp
    33bc:	89 ca                	mov    %ecx,%edx
    33be:	c1 e9 0c             	shr    $0xc,%ecx
    33c1:	c1 ea 0f             	shr    $0xf,%edx
    33c4:	83 e1 07             	and    $0x7,%ecx
    33c7:	d3 e0                	shl    %cl,%eax
    33c9:	08 82 40 6e 00 00    	or     %al,0x6e40(%edx)
    33cf:	c3                   	ret
    33d0:	55                   	push   %ebp
    33d1:	89 e5                	mov    %esp,%ebp
    33d3:	8b 4d 08             	mov    0x8(%ebp),%ecx
    33d6:	81 f9 ff ff 0f 00    	cmp    $0xfffff,%ecx
    33dc:	76 15                	jbe    0x33f3
    33de:	89 ca                	mov    %ecx,%edx
    33e0:	b8 fe ff ff ff       	mov    $0xfffffffe,%eax
    33e5:	c1 e9 0c             	shr    $0xc,%ecx
    33e8:	c1 ea 0f             	shr    $0xf,%edx
    33eb:	d2 c0                	rol    %cl,%al
    33ed:	20 82 40 6e 00 00    	and    %al,0x6e40(%edx)
    33f3:	5d                   	pop    %ebp
    33f4:	c3                   	ret
    33f5:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    33fc:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    3400:	55                   	push   %ebp
    3401:	89 e5                	mov    %esp,%ebp
    3403:	8b 4d 08             	mov    0x8(%ebp),%ecx
    3406:	5d                   	pop    %ebp
    3407:	89 c8                	mov    %ecx,%eax
    3409:	c1 e9 0c             	shr    $0xc,%ecx
    340c:	c1 e8 0f             	shr    $0xf,%eax
    340f:	83 e1 07             	and    $0x7,%ecx
    3412:	0f b6 80 40 6e 00 00 	movzbl 0x6e40(%eax),%eax
    3419:	d3 f8                	sar    %cl,%eax
    341b:	83 e0 01             	and    $0x1,%eax
    341e:	c3                   	ret
    341f:	90                   	nop
    3420:	55                   	push   %ebp
    3421:	0f b6 15 40 6e 00 00 	movzbl 0x6e40,%edx
    3428:	89 e5                	mov    %esp,%ebp
    342a:	57                   	push   %edi
    342b:	56                   	push   %esi
    342c:	53                   	push   %ebx
    342d:	f6 c2 01             	test   $0x1,%dl
    3430:	74 5a                	je     0x348c
    3432:	b8 00 10 00 00       	mov    $0x1000,%eax
    3437:	eb 0e                	jmp    0x3447
    3439:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    3440:	3d 00 00 00 02       	cmp    $0x2000000,%eax
    3445:	74 3c                	je     0x3483
    3447:	89 c3                	mov    %eax,%ebx
    3449:	89 c1                	mov    %eax,%ecx
    344b:	89 c7                	mov    %eax,%edi
    344d:	05 00 10 00 00       	add    $0x1000,%eax
    3452:	c1 eb 0f             	shr    $0xf,%ebx
    3455:	c1 e9 0c             	shr    $0xc,%ecx
    3458:	0f b6 b3 40 6e 00 00 	movzbl 0x6e40(%ebx),%esi
    345f:	83 e1 07             	and    $0x7,%ecx
    3462:	0f a3 ce             	bt     %ecx,%esi
    3465:	89 f2                	mov    %esi,%edx
    3467:	72 d7                	jb     0x3440
    3469:	be 01 00 00 00       	mov    $0x1,%esi
    346e:	89 f0                	mov    %esi,%eax
    3470:	d3 e0                	shl    %cl,%eax
    3472:	89 c1                	mov    %eax,%ecx
    3474:	09 ca                	or     %ecx,%edx
    3476:	89 f8                	mov    %edi,%eax
    3478:	88 93 40 6e 00 00    	mov    %dl,0x6e40(%ebx)
    347e:	5b                   	pop    %ebx
    347f:	5e                   	pop    %esi
    3480:	5f                   	pop    %edi
    3481:	5d                   	pop    %ebp
    3482:	c3                   	ret
    3483:	31 ff                	xor    %edi,%edi
    3485:	5b                   	pop    %ebx
    3486:	5e                   	pop    %esi
    3487:	89 f8                	mov    %edi,%eax
    3489:	5f                   	pop    %edi
    348a:	5d                   	pop    %ebp
    348b:	c3                   	ret
    348c:	b9 01 00 00 00       	mov    $0x1,%ecx
    3491:	31 ff                	xor    %edi,%edi
    3493:	31 db                	xor    %ebx,%ebx
    3495:	eb dd                	jmp    0x3474
    3497:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    349e:	66 90                	xchg   %ax,%ax
    34a0:	55                   	push   %ebp
    34a1:	b8 40 6e 00 00       	mov    $0x6e40,%eax
    34a6:	ba 40 72 00 00       	mov    $0x7240,%edx
    34ab:	89 e5                	mov    %esp,%ebp
    34ad:	56                   	push   %esi
    34ae:	53                   	push   %ebx
    34af:	90                   	nop
    34b0:	c7 00 00 00 00 00    	movl   $0x0,(%eax)
    34b6:	83 c0 08             	add    $0x8,%eax
    34b9:	c7 40 fc 00 00 00 00 	movl   $0x0,-0x4(%eax)
    34c0:	39 c2                	cmp    %eax,%edx
    34c2:	75 ec                	jne    0x34b0
    34c4:	31 c0                	xor    %eax,%eax
    34c6:	bb 01 00 00 00       	mov    $0x1,%ebx
    34cb:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    34cf:	90                   	nop
    34d0:	89 c1                	mov    %eax,%ecx
    34d2:	89 c2                	mov    %eax,%edx
    34d4:	89 de                	mov    %ebx,%esi
    34d6:	05 00 10 00 00       	add    $0x1000,%eax
    34db:	c1 e9 0c             	shr    $0xc,%ecx
    34de:	c1 ea 0f             	shr    $0xf,%edx
    34e1:	83 e1 07             	and    $0x7,%ecx
    34e4:	d3 e6                	shl    %cl,%esi
    34e6:	89 f1                	mov    %esi,%ecx
    34e8:	08 8a 40 6e 00 00    	or     %cl,0x6e40(%edx)
    34ee:	3d 00 00 10 00       	cmp    $0x100000,%eax
    34f3:	75 db                	jne    0x34d0
    34f5:	5b                   	pop    %ebx
    34f6:	5e                   	pop    %esi
    34f7:	5d                   	pop    %ebp
    34f8:	c3                   	ret
    34f9:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    3500:	55                   	push   %ebp
    3501:	89 e5                	mov    %esp,%ebp
    3503:	57                   	push   %edi
    3504:	56                   	push   %esi
    3505:	be 0b 00 00 00       	mov    $0xb,%esi
    350a:	53                   	push   %ebx
    350b:	bb 00 00 01 00       	mov    $0x10000,%ebx
    3510:	83 ec 1c             	sub    $0x1c,%esp
    3513:	6a 0e                	push   $0xe
    3515:	6a 00                	push   $0x0
    3517:	ff 35 34 59 00 00    	push   0x5934
    351d:	68 5c 58 00 00       	push   $0x585c
    3522:	e8 69 e5 ff ff       	call   0x1a90
    3527:	83 c4 10             	add    $0x10,%esp
    352a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    3530:	8d 83 00 00 ff ff    	lea    -0x10000(%ebx),%eax
    3536:	c1 e8 0f             	shr    $0xf,%eax
    3539:	f6 80 40 6e 00 00 01 	testb  $0x1,0x6e40(%eax)
    3540:	0f 85 92 01 00 00    	jne    0x36d8
    3546:	8d 83 00 10 ff ff    	lea    -0xf000(%ebx),%eax
    354c:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    3550:	89 c2                	mov    %eax,%edx
    3552:	c1 ea 0f             	shr    $0xf,%edx
    3555:	0f b6 8a 40 6e 00 00 	movzbl 0x6e40(%edx),%ecx
    355c:	89 c2                	mov    %eax,%edx
    355e:	05 00 10 00 00       	add    $0x1000,%eax
    3563:	c1 ea 0c             	shr    $0xc,%edx
    3566:	83 e2 07             	and    $0x7,%edx
    3569:	0f a3 d1             	bt     %edx,%ecx
    356c:	0f 82 66 01 00 00    	jb     0x36d8
    3572:	39 c3                	cmp    %eax,%ebx
    3574:	75 da                	jne    0x3550
    3576:	6a 07                	push   $0x7
    3578:	56                   	push   %esi
    3579:	ff 35 34 59 00 00    	push   0x5934
    357f:	68 74 58 00 00       	push   $0x5874
    3584:	e8 07 e5 ff ff       	call   0x1a90
    3589:	83 c4 10             	add    $0x10,%esp
    358c:	81 c3 00 00 01 00    	add    $0x10000,%ebx
    3592:	83 c6 01             	add    $0x1,%esi
    3595:	81 fb 00 00 11 00    	cmp    $0x110000,%ebx
    359b:	75 93                	jne    0x3530
    359d:	a1 34 59 00 00       	mov    0x5934,%eax
    35a2:	83 c0 01             	add    $0x1,%eax
    35a5:	a3 34 59 00 00       	mov    %eax,0x5934
    35aa:	83 f8 17             	cmp    $0x17,%eax
    35ad:	0f 8f 40 01 00 00    	jg     0x36f3
    35b3:	6a 0e                	push   $0xe
    35b5:	bf 0b 00 00 00       	mov    $0xb,%edi
    35ba:	31 db                	xor    %ebx,%ebx
    35bc:	31 f6                	xor    %esi,%esi
    35be:	6a 00                	push   $0x0
    35c0:	50                   	push   %eax
    35c1:	68 69 58 00 00       	push   $0x5869
    35c6:	e8 c5 e4 ff ff       	call   0x1a90
    35cb:	83 c4 10             	add    $0x10,%esp
    35ce:	eb 1c                	jmp    0x35ec
    35d0:	6a 0a                	push   $0xa
    35d2:	83 c6 01             	add    $0x1,%esi
    35d5:	57                   	push   %edi
    35d6:	83 c7 01             	add    $0x1,%edi
    35d9:	52                   	push   %edx
    35da:	68 67 58 00 00       	push   $0x5867
    35df:	e8 ac e4 ff ff       	call   0x1a90
    35e4:	83 c4 10             	add    $0x10,%esp
    35e7:	83 ff 47             	cmp    $0x47,%edi
    35ea:	74 43                	je     0x362f
    35ec:	89 f8                	mov    %edi,%eax
    35ee:	8b 15 34 59 00 00    	mov    0x5934,%edx
    35f4:	c1 e0 0c             	shl    $0xc,%eax
    35f7:	05 00 50 0f 00       	add    $0xf5000,%eax
    35fc:	89 c1                	mov    %eax,%ecx
    35fe:	c1 e8 0c             	shr    $0xc,%eax
    3601:	c1 e9 0f             	shr    $0xf,%ecx
    3604:	83 e0 07             	and    $0x7,%eax
    3607:	0f b6 89 40 6e 00 00 	movzbl 0x6e40(%ecx),%ecx
    360e:	0f a3 c1             	bt     %eax,%ecx
    3611:	72 bd                	jb     0x35d0
    3613:	6a 02                	push   $0x2
    3615:	83 c3 01             	add    $0x1,%ebx
    3618:	57                   	push   %edi
    3619:	83 c7 01             	add    $0x1,%edi
    361c:	52                   	push   %edx
    361d:	68 74 58 00 00       	push   $0x5874
    3622:	e8 69 e4 ff ff       	call   0x1a90
    3627:	83 c4 10             	add    $0x10,%esp
    362a:	83 ff 47             	cmp    $0x47,%edi
    362d:	75 bd                	jne    0x35ec
    362f:	a1 34 59 00 00       	mov    0x5934,%eax
    3634:	83 c0 01             	add    $0x1,%eax
    3637:	a3 34 59 00 00       	mov    %eax,0x5934
    363c:	83 f8 17             	cmp    $0x17,%eax
    363f:	0f 8f d9 00 00 00    	jg     0x371e
    3645:	6a 0a                	push   $0xa
    3647:	6a 00                	push   $0x0
    3649:	50                   	push   %eax
    364a:	68 76 58 00 00       	push   $0x5876
    364f:	e8 3c e4 ff ff       	call   0x1a90
    3654:	83 c4 10             	add    $0x10,%esp
    3657:	85 f6                	test   %esi,%esi
    3659:	0f 84 ae 00 00 00    	je     0x370d
    365f:	8d 7d dc             	lea    -0x24(%ebp),%edi
    3662:	89 f0                	mov    %esi,%eax
    3664:	89 fa                	mov    %edi,%edx
    3666:	e8 c5 fc ff ff       	call   0x3330
    366b:	6a 0f                	push   $0xf
    366d:	6a 05                	push   $0x5
    366f:	ff 35 34 59 00 00    	push   0x5934
    3675:	57                   	push   %edi
    3676:	e8 15 e4 ff ff       	call   0x1a90
    367b:	6a 07                	push   $0x7
    367d:	6a 0a                	push   $0xa
    367f:	ff 35 34 59 00 00    	push   0x5934
    3685:	68 7c 58 00 00       	push   $0x587c
    368a:	e8 01 e4 ff ff       	call   0x1a90
    368f:	83 c4 20             	add    $0x20,%esp
    3692:	85 db                	test   %ebx,%ebx
    3694:	74 6c                	je     0x3702
    3696:	89 fa                	mov    %edi,%edx
    3698:	89 d8                	mov    %ebx,%eax
    369a:	e8 91 fc ff ff       	call   0x3330
    369f:	6a 0f                	push   $0xf
    36a1:	6a 0f                	push   $0xf
    36a3:	ff 35 34 59 00 00    	push   0x5934
    36a9:	57                   	push   %edi
    36aa:	e8 e1 e3 ff ff       	call   0x1a90
    36af:	6a 08                	push   $0x8
    36b1:	6a 14                	push   $0x14
    36b3:	ff 35 34 59 00 00    	push   0x5934
    36b9:	68 82 58 00 00       	push   $0x5882
    36be:	e8 cd e3 ff ff       	call   0x1a90
    36c3:	83 05 34 59 00 00 01 	addl   $0x1,0x5934
    36ca:	8d 65 f4             	lea    -0xc(%ebp),%esp
    36cd:	5b                   	pop    %ebx
    36ce:	5e                   	pop    %esi
    36cf:	5f                   	pop    %edi
    36d0:	5d                   	pop    %ebp
    36d1:	c3                   	ret
    36d2:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    36d8:	6a 0c                	push   $0xc
    36da:	56                   	push   %esi
    36db:	ff 35 34 59 00 00    	push   0x5934
    36e1:	68 67 58 00 00       	push   $0x5867
    36e6:	e8 a5 e3 ff ff       	call   0x1a90
    36eb:	83 c4 10             	add    $0x10,%esp
    36ee:	e9 99 fe ff ff       	jmp    0x358c
    36f3:	e8 a8 da ff ff       	call   0x11a0
    36f8:	a1 34 59 00 00       	mov    0x5934,%eax
    36fd:	e9 b1 fe ff ff       	jmp    0x35b3
    3702:	b8 30 00 00 00       	mov    $0x30,%eax
    3707:	66 89 45 dc          	mov    %ax,-0x24(%ebp)
    370b:	eb 92                	jmp    0x369f
    370d:	ba 30 00 00 00       	mov    $0x30,%edx
    3712:	8d 7d dc             	lea    -0x24(%ebp),%edi
    3715:	66 89 55 dc          	mov    %dx,-0x24(%ebp)
    3719:	e9 4d ff ff ff       	jmp    0x366b
    371e:	e8 7d da ff ff       	call   0x11a0
    3723:	a1 34 59 00 00       	mov    0x5934,%eax
    3728:	e9 18 ff ff ff       	jmp    0x3645
    372d:	8d 76 00             	lea    0x0(%esi),%esi
    3730:	55                   	push   %ebp
    3731:	ba 00 00 10 00       	mov    $0x100000,%edx
    3736:	89 e5                	mov    %esp,%ebp
    3738:	53                   	push   %ebx
    3739:	31 db                	xor    %ebx,%ebx
    373b:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    373f:	90                   	nop
    3740:	89 d0                	mov    %edx,%eax
    3742:	89 d1                	mov    %edx,%ecx
    3744:	c1 e8 0f             	shr    $0xf,%eax
    3747:	c1 e9 0c             	shr    $0xc,%ecx
    374a:	0f b6 80 40 6e 00 00 	movzbl 0x6e40(%eax),%eax
    3751:	83 e1 07             	and    $0x7,%ecx
    3754:	d3 f8                	sar    %cl,%eax
    3756:	83 e0 01             	and    $0x1,%eax
    3759:	83 f8 01             	cmp    $0x1,%eax
    375c:	83 d3 00             	adc    $0x0,%ebx
    375f:	81 c2 00 10 00 00    	add    $0x1000,%edx
    3765:	81 fa 00 00 00 02    	cmp    $0x2000000,%edx
    376b:	75 d3                	jne    0x3740
    376d:	89 d8                	mov    %ebx,%eax
    376f:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    3772:	c9                   	leave
    3773:	c3                   	ret
    3774:	66 90                	xchg   %ax,%ax
    3776:	66 90                	xchg   %ax,%ax
    3778:	66 90                	xchg   %ax,%ax
    377a:	66 90                	xchg   %ax,%ax
    377c:	66 90                	xchg   %ax,%ax
    377e:	66 90                	xchg   %ax,%ax
    3780:	55                   	push   %ebp
    3781:	89 e5                	mov    %esp,%ebp
    3783:	56                   	push   %esi
    3784:	53                   	push   %ebx
    3785:	8b 5d 0c             	mov    0xc(%ebp),%ebx
    3788:	89 de                	mov    %ebx,%esi
    378a:	c1 eb 0c             	shr    $0xc,%ebx
    378d:	c1 ee 16             	shr    $0x16,%esi
    3790:	81 e3 ff 03 00 00    	and    $0x3ff,%ebx
    3796:	8b 04 b5 00 a0 00 00 	mov    0xa000(,%esi,4),%eax
    379d:	89 c1                	mov    %eax,%ecx
    379f:	81 e1 00 f0 ff ff    	and    $0xfffff000,%ecx
    37a5:	a8 01                	test   $0x1,%al
    37a7:	74 12                	je     0x37bb
    37a9:	8b 45 08             	mov    0x8(%ebp),%eax
    37ac:	25 00 f0 ff ff       	and    $0xfffff000,%eax
    37b1:	83 c8 03             	or     $0x3,%eax
    37b4:	89 04 99             	mov    %eax,(%ecx,%ebx,4)
    37b7:	5b                   	pop    %ebx
    37b8:	5e                   	pop    %esi
    37b9:	5d                   	pop    %ebp
    37ba:	c3                   	ret
    37bb:	e8 60 fc ff ff       	call   0x3420
    37c0:	89 c1                	mov    %eax,%ecx
    37c2:	8d 90 00 10 00 00    	lea    0x1000(%eax),%edx
    37c8:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    37cf:	90                   	nop
    37d0:	c7 00 00 00 00 00    	movl   $0x0,(%eax)
    37d6:	83 c0 08             	add    $0x8,%eax
    37d9:	c7 40 fc 00 00 00 00 	movl   $0x0,-0x4(%eax)
    37e0:	39 d0                	cmp    %edx,%eax
    37e2:	75 ec                	jne    0x37d0
    37e4:	89 c8                	mov    %ecx,%eax
    37e6:	83 c8 03             	or     $0x3,%eax
    37e9:	89 04 b5 00 a0 00 00 	mov    %eax,0xa000(,%esi,4)
    37f0:	eb b7                	jmp    0x37a9
    37f2:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    37f9:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    3800:	55                   	push   %ebp
    3801:	31 c0                	xor    %eax,%eax
    3803:	89 e5                	mov    %esp,%ebp
    3805:	53                   	push   %ebx
    3806:	83 ec 04             	sub    $0x4,%esp
    3809:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    3810:	c7 04 85 00 a0 00 00 	movl   $0x2,0xa000(,%eax,4)
    3817:	02 00 00 00 
    381b:	c7 04 85 04 a0 00 00 	movl   $0x2,0xa004(,%eax,4)
    3822:	02 00 00 00 
    3826:	83 c0 02             	add    $0x2,%eax
    3829:	3d 00 04 00 00       	cmp    $0x400,%eax
    382e:	75 e0                	jne    0x3810
    3830:	31 c0                	xor    %eax,%eax
    3832:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    3838:	89 c2                	mov    %eax,%edx
    383a:	c1 e2 0c             	shl    $0xc,%edx
    383d:	83 ca 03             	or     $0x3,%edx
    3840:	89 14 85 00 90 00 00 	mov    %edx,0x9000(,%eax,4)
    3847:	83 c0 01             	add    $0x1,%eax
    384a:	3d 00 04 00 00       	cmp    $0x400,%eax
    384f:	75 e7                	jne    0x3838
    3851:	b8 00 90 00 00       	mov    $0x9000,%eax
    3856:	ba 00 00 40 00       	mov    $0x400000,%edx
    385b:	83 c8 03             	or     $0x3,%eax
    385e:	a3 00 a0 00 00       	mov    %eax,0xa000
    3863:	31 c0                	xor    %eax,%eax
    3865:	8d 76 00             	lea    0x0(%esi),%esi
    3868:	89 d1                	mov    %edx,%ecx
    386a:	81 c2 00 10 00 00    	add    $0x1000,%edx
    3870:	83 c9 03             	or     $0x3,%ecx
    3873:	89 0c 85 00 80 00 00 	mov    %ecx,0x8000(,%eax,4)
    387a:	83 c0 01             	add    $0x1,%eax
    387d:	3d 00 04 00 00       	cmp    $0x400,%eax
    3882:	75 e4                	jne    0x3868
    3884:	83 ec 0c             	sub    $0xc,%esp
    3887:	b8 00 80 00 00       	mov    $0x8000,%eax
    388c:	bb 00 00 00 fd       	mov    $0xfd000000,%ebx
    3891:	68 00 a0 00 00       	push   $0xa000
    3896:	83 c8 03             	or     $0x3,%eax
    3899:	a3 04 a0 00 00       	mov    %eax,0xa004
    389e:	e8 5d 00 00 00       	call   0x3900
    38a3:	e8 65 00 00 00       	call   0x390d
    38a8:	83 c4 10             	add    $0x10,%esp
    38ab:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    38af:	90                   	nop
    38b0:	83 ec 08             	sub    $0x8,%esp
    38b3:	53                   	push   %ebx
    38b4:	53                   	push   %ebx
    38b5:	81 c3 00 10 00 00    	add    $0x1000,%ebx
    38bb:	e8 c0 fe ff ff       	call   0x3780
    38c0:	83 c4 10             	add    $0x10,%esp
    38c3:	81 fb 00 00 20 fd    	cmp    $0xfd200000,%ebx
    38c9:	75 e5                	jne    0x38b0
    38cb:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    38ce:	c9                   	leave
    38cf:	c3                   	ret
    38d0:	55                   	push   %ebp
    38d1:	89 e5                	mov    %esp,%ebp
    38d3:	56                   	push   %esi
    38d4:	53                   	push   %ebx
    38d5:	8b 5d 08             	mov    0x8(%ebp),%ebx
    38d8:	8d b3 00 00 20 00    	lea    0x200000(%ebx),%esi
    38de:	66 90                	xchg   %ax,%ax
    38e0:	83 ec 08             	sub    $0x8,%esp
    38e3:	53                   	push   %ebx
    38e4:	53                   	push   %ebx
    38e5:	81 c3 00 10 00 00    	add    $0x1000,%ebx
    38eb:	e8 90 fe ff ff       	call   0x3780
    38f0:	83 c4 10             	add    $0x10,%esp
    38f3:	39 f3                	cmp    %esi,%ebx
    38f5:	75 e9                	jne    0x38e0
    38f7:	8d 65 f8             	lea    -0x8(%ebp),%esp
    38fa:	5b                   	pop    %ebx
    38fb:	5e                   	pop    %esi
    38fc:	5d                   	pop    %ebp
    38fd:	c3                   	ret
    38fe:	66 90                	xchg   %ax,%ax
    3900:	55                   	push   %ebp
    3901:	89 e5                	mov    %esp,%ebp
    3903:	8b 45 08             	mov    0x8(%ebp),%eax
    3906:	0f 22 d8             	mov    %eax,%cr3
    3909:	89 ec                	mov    %ebp,%esp
    390b:	5d                   	pop    %ebp
    390c:	c3                   	ret
    390d:	55                   	push   %ebp
    390e:	89 e5                	mov    %esp,%ebp
    3910:	0f 20 c0             	mov    %cr0,%eax
    3913:	0d 01 00 00 80       	or     $0x80000001,%eax
    3918:	0f 22 c0             	mov    %eax,%cr0
    391b:	89 ec                	mov    %ebp,%esp
    391d:	5d                   	pop    %ebp
    391e:	c3                   	ret
    391f:	90                   	nop
    3920:	60                   	pusha
    3921:	e8 4a d2 ff ff       	call   0xb70
    3926:	b0 20                	mov    $0x20,%al
    3928:	e6 20                	out    %al,$0x20
    392a:	61                   	popa
    392b:	cf                   	iret
    392c:	60                   	pusha
    392d:	e8 be f4 ff ff       	call   0x2df0
    3932:	b0 20                	mov    $0x20,%al
    3934:	e6 20                	out    %al,$0x20
    3936:	61                   	popa
    3937:	cf                   	iret
    3938:	60                   	pusha
    3939:	e8 42 00 00 00       	call   0x3980
    393e:	b0 20                	mov    $0x20,%al
    3940:	e6 20                	out    %al,$0x20
    3942:	61                   	popa
    3943:	cf                   	iret
    3944:	60                   	pusha
    3945:	54                   	push   %esp
    3946:	e8 05 01 00 00       	call   0x3a50
    394b:	83 c4 04             	add    $0x4,%esp
    394e:	61                   	popa
    394f:	cf                   	iret
    3950:	fa                   	cli
    3951:	e8 3a cc ff ff       	call   0x590
    3956:	f4                   	hlt
    3957:	eb fd                	jmp    0x3956
    3959:	66 90                	xchg   %ax,%ax
    395b:	66 90                	xchg   %ax,%ax
    395d:	66 90                	xchg   %ax,%ax
    395f:	90                   	nop
    3960:	55                   	push   %ebp
    3961:	57                   	push   %edi
    3962:	56                   	push   %esi
    3963:	53                   	push   %ebx
    3964:	8b 44 24 14          	mov    0x14(%esp),%eax
    3968:	89 20                	mov    %esp,(%eax)
    396a:	8b 44 24 18          	mov    0x18(%esp),%eax
    396e:	8b 20                	mov    (%eax),%esp
    3970:	5b                   	pop    %ebx
    3971:	5e                   	pop    %esi
    3972:	5f                   	pop    %edi
    3973:	5d                   	pop    %ebp
    3974:	c3                   	ret
    3975:	66 90                	xchg   %ax,%ax
    3977:	66 90                	xchg   %ax,%ax
    3979:	66 90                	xchg   %ax,%ax
    397b:	66 90                	xchg   %ax,%ax
    397d:	66 90                	xchg   %ax,%ax
    397f:	90                   	nop
    3980:	55                   	push   %ebp
    3981:	89 e5                	mov    %esp,%ebp
    3983:	83 ec 10             	sub    $0x10,%esp
    3986:	8b 15 00 b0 00 00    	mov    0xb000,%edx
    398c:	8d 42 01             	lea    0x1(%edx),%eax
    398f:	89 c1                	mov    %eax,%ecx
    3991:	c1 e9 1f             	shr    $0x1f,%ecx
    3994:	01 c8                	add    %ecx,%eax
    3996:	83 e0 01             	and    $0x1,%eax
    3999:	29 c8                	sub    %ecx,%eax
    399b:	a3 00 b0 00 00       	mov    %eax,0xb000
    39a0:	8d 04 40             	lea    (%eax,%eax,2),%eax
    39a3:	8d 04 c5 20 b0 00 00 	lea    0xb020(,%eax,8),%eax
    39aa:	50                   	push   %eax
    39ab:	8d 04 52             	lea    (%edx,%edx,2),%eax
    39ae:	8d 04 c5 20 b0 00 00 	lea    0xb020(,%eax,8),%eax
    39b5:	50                   	push   %eax
    39b6:	e8 a5 ff ff ff       	call   0x3960
    39bb:	83 c4 10             	add    $0x10,%esp
    39be:	c9                   	leave
    39bf:	c3                   	ret
    39c0:	55                   	push   %ebp
    39c1:	89 e5                	mov    %esp,%ebp
    39c3:	53                   	push   %ebx
    39c4:	83 ec 04             	sub    $0x4,%esp
    39c7:	8b 5d 08             	mov    0x8(%ebp),%ebx
    39ca:	e8 51 fa ff ff       	call   0x3420
    39cf:	8b 4d 0c             	mov    0xc(%ebp),%ecx
    39d2:	8d 14 5b             	lea    (%ebx,%ebx,2),%edx
    39d5:	05 ec 0f 00 00       	add    $0xfec,%eax
    39da:	c7 40 0c 00 00 00 00 	movl   $0x0,0xc(%eax)
    39e1:	c1 e2 03             	shl    $0x3,%edx
    39e4:	89 48 10             	mov    %ecx,0x10(%eax)
    39e7:	c7 40 08 00 00 00 00 	movl   $0x0,0x8(%eax)
    39ee:	c7 40 04 00 00 00 00 	movl   $0x0,0x4(%eax)
    39f5:	c7 00 00 00 00 00    	movl   $0x0,(%eax)
    39fb:	89 82 20 b0 00 00    	mov    %eax,0xb020(%edx)
    3a01:	8b 45 10             	mov    0x10(%ebp),%eax
    3a04:	89 9a 34 b0 00 00    	mov    %ebx,0xb034(%edx)
    3a0a:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    3a0d:	89 82 2c b0 00 00    	mov    %eax,0xb02c(%edx)
    3a13:	c7 82 30 b0 00 00 01 	movl   $0x1,0xb030(%edx)
    3a1a:	00 00 00 
    3a1d:	c9                   	leave
    3a1e:	c3                   	ret
    3a1f:	90                   	nop
    3a20:	55                   	push   %ebp
    3a21:	89 e5                	mov    %esp,%ebp
    3a23:	83 ec 08             	sub    $0x8,%esp
    3a26:	68 ee 00 00 00       	push   $0xee
    3a2b:	6a 08                	push   $0x8
    3a2d:	68 44 49 00 00       	push   $0x4944
    3a32:	68 80 00 00 00       	push   $0x80
    3a37:	e8 14 cb ff ff       	call   0x550
    3a3c:	83 c4 10             	add    $0x10,%esp
    3a3f:	c9                   	leave
    3a40:	c3                   	ret
    3a41:	66 90                	xchg   %ax,%ax
    3a43:	66 90                	xchg   %ax,%ax
    3a45:	66 90                	xchg   %ax,%ax
    3a47:	66 90                	xchg   %ax,%ax
    3a49:	66 90                	xchg   %ax,%ax
    3a4b:	66 90                	xchg   %ax,%ax
    3a4d:	66 90                	xchg   %ax,%ax
    3a4f:	90                   	nop
    3a50:	55                   	push   %ebp
    3a51:	89 e5                	mov    %esp,%ebp
    3a53:	53                   	push   %ebx
    3a54:	83 ec 04             	sub    $0x4,%esp
    3a57:	8b 5d 08             	mov    0x8(%ebp),%ebx
    3a5a:	8b 43 1c             	mov    0x1c(%ebx),%eax
    3a5d:	83 f8 05             	cmp    $0x5,%eax
    3a60:	74 4e                	je     0x3ab0
    3a62:	83 f8 0a             	cmp    $0xa,%eax
    3a65:	74 21                	je     0x3a88
    3a67:	83 f8 04             	cmp    $0x4,%eax
    3a6a:	75 13                	jne    0x3a7f
    3a6c:	83 ec 08             	sub    $0x8,%esp
    3a6f:	68 00 ff 00 00       	push   $0xff00
    3a74:	ff 73 18             	push   0x18(%ebx)
    3a77:	e8 74 03 00 00       	call   0x3df0
    3a7c:	83 c4 10             	add    $0x10,%esp
    3a7f:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    3a82:	c9                   	leave
    3a83:	c3                   	ret
    3a84:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    3a88:	fa                   	cli
    3a89:	c7 05 e4 6d 00 00 01 	movl   $0x1,0x6de4
    3a90:	00 00 00 
    3a93:	e8 a8 00 00 00       	call   0x3b40
    3a98:	c7 05 e4 6d 00 00 00 	movl   $0x0,0x6de4
    3a9f:	00 00 00 
    3aa2:	fb                   	sti
    3aa3:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    3aa6:	c9                   	leave
    3aa7:	c3                   	ret
    3aa8:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    3aaf:	90                   	nop
    3ab0:	8b 15 ec 6d 00 00    	mov    0x6dec,%edx
    3ab6:	a1 e8 6d 00 00       	mov    0x6de8,%eax
    3abb:	39 c2                	cmp    %eax,%edx
    3abd:	74 31                	je     0x3af0
    3abf:	a1 e8 6d 00 00       	mov    0x6de8,%eax
    3ac4:	0f b6 88 00 6e 00 00 	movzbl 0x6e00(%eax),%ecx
    3acb:	a1 e8 6d 00 00       	mov    0x6de8,%eax
    3ad0:	83 c0 01             	add    $0x1,%eax
    3ad3:	89 4b 1c             	mov    %ecx,0x1c(%ebx)
    3ad6:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    3ad9:	99                   	cltd
    3ada:	c1 ea 1b             	shr    $0x1b,%edx
    3add:	01 d0                	add    %edx,%eax
    3adf:	83 e0 1f             	and    $0x1f,%eax
    3ae2:	29 d0                	sub    %edx,%eax
    3ae4:	a3 e8 6d 00 00       	mov    %eax,0x6de8
    3ae9:	c9                   	leave
    3aea:	c3                   	ret
    3aeb:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    3aef:	90                   	nop
    3af0:	e8 8b fe ff ff       	call   0x3980
    3af5:	c7 43 1c 00 00 00 00 	movl   $0x0,0x1c(%ebx)
    3afc:	eb 81                	jmp    0x3a7f
    3afe:	66 90                	xchg   %ax,%ax
    3b00:	55                   	push   %ebp
    3b01:	89 e5                	mov    %esp,%ebp
    3b03:	8b 55 08             	mov    0x8(%ebp),%edx
    3b06:	81 fa 1f 03 00 00    	cmp    $0x31f,%edx
    3b0c:	77 09                	ja     0x3b17
    3b0e:	81 7d 0c 57 02 00 00 	cmpl   $0x257,0xc(%ebp)
    3b15:	76 09                	jbe    0x3b20
    3b17:	5d                   	pop    %ebp
    3b18:	c3                   	ret
    3b19:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    3b20:	69 45 0c 20 03 00 00 	imul   $0x320,0xc(%ebp),%eax
    3b27:	01 d0                	add    %edx,%eax
    3b29:	8b 55 10             	mov    0x10(%ebp),%edx
    3b2c:	5d                   	pop    %ebp
    3b2d:	89 14 85 80 b0 00 00 	mov    %edx,0xb080(,%eax,4)
    3b34:	c3                   	ret
    3b35:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    3b3c:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    3b40:	55                   	push   %ebp
    3b41:	b9 00 53 07 00       	mov    $0x75300,%ecx
    3b46:	89 e5                	mov    %esp,%ebp
    3b48:	57                   	push   %edi
    3b49:	8b 3d 28 59 00 00    	mov    0x5928,%edi
    3b4f:	56                   	push   %esi
    3b50:	be 80 b0 00 00       	mov    $0xb080,%esi
    3b55:	fc                   	cld
    3b56:	f3 a5                	rep movsl %ds:(%esi),%es:(%edi)
    3b58:	5e                   	pop    %esi
    3b59:	5f                   	pop    %edi
    3b5a:	5d                   	pop    %ebp
    3b5b:	c3                   	ret
    3b5c:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    3b60:	55                   	push   %ebp
    3b61:	31 c0                	xor    %eax,%eax
    3b63:	89 e5                	mov    %esp,%ebp
    3b65:	8b 55 08             	mov    0x8(%ebp),%edx
    3b68:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    3b6f:	90                   	nop
    3b70:	89 14 85 80 b0 00 00 	mov    %edx,0xb080(,%eax,4)
    3b77:	89 14 85 84 b0 00 00 	mov    %edx,0xb084(,%eax,4)
    3b7e:	83 c0 02             	add    $0x2,%eax
    3b81:	3d 00 53 07 00       	cmp    $0x75300,%eax
    3b86:	75 e8                	jne    0x3b70
    3b88:	5d                   	pop    %ebp
    3b89:	c3                   	ret
    3b8a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    3b90:	55                   	push   %ebp
    3b91:	89 e5                	mov    %esp,%ebp
    3b93:	57                   	push   %edi
    3b94:	56                   	push   %esi
    3b95:	53                   	push   %ebx
    3b96:	83 ec 04             	sub    $0x4,%esp
    3b99:	8b 7d 08             	mov    0x8(%ebp),%edi
    3b9c:	8b 75 0c             	mov    0xc(%ebp),%esi
    3b9f:	8b 4d 10             	mov    0x10(%ebp),%ecx
    3ba2:	8b 5d 18             	mov    0x18(%ebp),%ebx
    3ba5:	3b 75 14             	cmp    0x14(%ebp),%esi
    3ba8:	7d 61                	jge    0x3c0b
    3baa:	81 fe 57 02 00 00    	cmp    $0x257,%esi
    3bb0:	7f 59                	jg     0x3c0b
    3bb2:	39 cf                	cmp    %ecx,%edi
    3bb4:	0f 9c c2             	setl   %dl
    3bb7:	81 ff 1f 03 00 00    	cmp    $0x31f,%edi
    3bbd:	0f 9e c0             	setle  %al
    3bc0:	21 c2                	and    %eax,%edx
    3bc2:	88 55 f3             	mov    %dl,-0xd(%ebp)
    3bc5:	69 d6 80 0c 00 00    	imul   $0xc80,%esi,%edx
    3bcb:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    3bcf:	90                   	nop
    3bd0:	80 7d f3 00          	cmpb   $0x0,-0xd(%ebp)
    3bd4:	89 f8                	mov    %edi,%eax
    3bd6:	74 1d                	je     0x3bf5
    3bd8:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    3bdf:	90                   	nop
    3be0:	89 9c 82 80 b0 00 00 	mov    %ebx,0xb080(%edx,%eax,4)
    3be7:	83 c0 01             	add    $0x1,%eax
    3bea:	39 c1                	cmp    %eax,%ecx
    3bec:	7e 07                	jle    0x3bf5
    3bee:	3d 1f 03 00 00       	cmp    $0x31f,%eax
    3bf3:	7e eb                	jle    0x3be0
    3bf5:	83 c6 01             	add    $0x1,%esi
    3bf8:	81 c2 80 0c 00 00    	add    $0xc80,%edx
    3bfe:	39 75 14             	cmp    %esi,0x14(%ebp)
    3c01:	7e 08                	jle    0x3c0b
    3c03:	81 fe 57 02 00 00    	cmp    $0x257,%esi
    3c09:	7e c5                	jle    0x3bd0
    3c0b:	83 c4 04             	add    $0x4,%esp
    3c0e:	5b                   	pop    %ebx
    3c0f:	5e                   	pop    %esi
    3c10:	5f                   	pop    %edi
    3c11:	5d                   	pop    %ebp
    3c12:	c3                   	ret
    3c13:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    3c1a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    3c20:	55                   	push   %ebp
    3c21:	89 e5                	mov    %esp,%ebp
    3c23:	53                   	push   %ebx
    3c24:	8b 4d 08             	mov    0x8(%ebp),%ecx
    3c27:	8b 45 0c             	mov    0xc(%ebp),%eax
    3c2a:	8b 55 10             	mov    0x10(%ebp),%edx
    3c2d:	8b 5d 14             	mov    0x14(%ebp),%ebx
    3c30:	81 f9 57 02 00 00    	cmp    $0x257,%ecx
    3c36:	77 31                	ja     0x3c69
    3c38:	39 d0                	cmp    %edx,%eax
    3c3a:	7d 2d                	jge    0x3c69
    3c3c:	3d 1f 03 00 00       	cmp    $0x31f,%eax
    3c41:	7f 26                	jg     0x3c69
    3c43:	69 c9 80 0c 00 00    	imul   $0xc80,%ecx,%ecx
    3c49:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    3c50:	85 c0                	test   %eax,%eax
    3c52:	78 07                	js     0x3c5b
    3c54:	89 9c 81 80 b0 00 00 	mov    %ebx,0xb080(%ecx,%eax,4)
    3c5b:	83 c0 01             	add    $0x1,%eax
    3c5e:	39 c2                	cmp    %eax,%edx
    3c60:	7e 07                	jle    0x3c69
    3c62:	3d 1f 03 00 00       	cmp    $0x31f,%eax
    3c67:	7e e7                	jle    0x3c50
    3c69:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    3c6c:	c9                   	leave
    3c6d:	c3                   	ret
    3c6e:	66 90                	xchg   %ax,%ax
    3c70:	55                   	push   %ebp
    3c71:	89 e5                	mov    %esp,%ebp
    3c73:	56                   	push   %esi
    3c74:	8b 75 08             	mov    0x8(%ebp),%esi
    3c77:	8b 55 0c             	mov    0xc(%ebp),%edx
    3c7a:	53                   	push   %ebx
    3c7b:	8b 4d 10             	mov    0x10(%ebp),%ecx
    3c7e:	8b 5d 14             	mov    0x14(%ebp),%ebx
    3c81:	81 fe 1f 03 00 00    	cmp    $0x31f,%esi
    3c87:	77 3d                	ja     0x3cc6
    3c89:	39 d1                	cmp    %edx,%ecx
    3c8b:	89 d0                	mov    %edx,%eax
    3c8d:	0f 4e c1             	cmovle %ecx,%eax
    3c90:	0f 4c ca             	cmovl  %edx,%ecx
    3c93:	39 c8                	cmp    %ecx,%eax
    3c95:	7f 2f                	jg     0x3cc6
    3c97:	69 d0 20 03 00 00    	imul   $0x320,%eax,%edx
    3c9d:	83 c1 01             	add    $0x1,%ecx
    3ca0:	01 f2                	add    %esi,%edx
    3ca2:	8d 14 95 80 b0 00 00 	lea    0xb080(,%edx,4),%edx
    3ca9:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    3cb0:	3d 57 02 00 00       	cmp    $0x257,%eax
    3cb5:	77 02                	ja     0x3cb9
    3cb7:	89 1a                	mov    %ebx,(%edx)
    3cb9:	83 c0 01             	add    $0x1,%eax
    3cbc:	81 c2 80 0c 00 00    	add    $0xc80,%edx
    3cc2:	39 c8                	cmp    %ecx,%eax
    3cc4:	75 ea                	jne    0x3cb0
    3cc6:	5b                   	pop    %ebx
    3cc7:	5e                   	pop    %esi
    3cc8:	5d                   	pop    %ebp
    3cc9:	c3                   	ret
    3cca:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    3cd0:	55                   	push   %ebp
    3cd1:	89 e5                	mov    %esp,%ebp
    3cd3:	57                   	push   %edi
    3cd4:	56                   	push   %esi
    3cd5:	53                   	push   %ebx
    3cd6:	83 ec 0c             	sub    $0xc,%esp
    3cd9:	8b 5d 10             	mov    0x10(%ebp),%ebx
    3cdc:	69 f3 20 03 00 00    	imul   $0x320,%ebx,%esi
    3ce2:	8d 43 08             	lea    0x8(%ebx),%eax
    3ce5:	89 45 e8             	mov    %eax,-0x18(%ebp)
    3ce8:	0f be 45 08          	movsbl 0x8(%ebp),%eax
    3cec:	c1 e0 03             	shl    $0x3,%eax
    3cef:	29 d8                	sub    %ebx,%eax
    3cf1:	89 45 ec             	mov    %eax,-0x14(%ebp)
    3cf4:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    3cf8:	8b 45 ec             	mov    -0x14(%ebp),%eax
    3cfb:	8b 55 0c             	mov    0xc(%ebp),%edx
    3cfe:	89 75 f0             	mov    %esi,-0x10(%ebp)
    3d01:	0f b6 8c 18 80 59 00 	movzbl 0x5980(%eax,%ebx,1),%ecx
    3d08:	00 
    3d09:	b8 07 00 00 00       	mov    $0x7,%eax
    3d0e:	eb 0b                	jmp    0x3d1b
    3d10:	83 e8 01             	sub    $0x1,%eax
    3d13:	83 c2 01             	add    $0x1,%edx
    3d16:	83 f8 ff             	cmp    $0xffffffff,%eax
    3d19:	74 35                	je     0x3d50
    3d1b:	0f a3 c1             	bt     %eax,%ecx
    3d1e:	73 f0                	jae    0x3d10
    3d20:	81 fa 1f 03 00 00    	cmp    $0x31f,%edx
    3d26:	77 e8                	ja     0x3d10
    3d28:	81 fb 57 02 00 00    	cmp    $0x257,%ebx
    3d2e:	77 e0                	ja     0x3d10
    3d30:	8b 7d f0             	mov    -0x10(%ebp),%edi
    3d33:	8b 75 14             	mov    0x14(%ebp),%esi
    3d36:	83 e8 01             	sub    $0x1,%eax
    3d39:	c1 e7 02             	shl    $0x2,%edi
    3d3c:	89 b4 97 80 b0 00 00 	mov    %esi,0xb080(%edi,%edx,4)
    3d43:	83 c2 01             	add    $0x1,%edx
    3d46:	83 f8 ff             	cmp    $0xffffffff,%eax
    3d49:	75 d0                	jne    0x3d1b
    3d4b:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    3d4f:	90                   	nop
    3d50:	8b 75 f0             	mov    -0x10(%ebp),%esi
    3d53:	8b 45 e8             	mov    -0x18(%ebp),%eax
    3d56:	83 c3 01             	add    $0x1,%ebx
    3d59:	81 c6 20 03 00 00    	add    $0x320,%esi
    3d5f:	39 c3                	cmp    %eax,%ebx
    3d61:	75 95                	jne    0x3cf8
    3d63:	83 c4 0c             	add    $0xc,%esp
    3d66:	5b                   	pop    %ebx
    3d67:	5e                   	pop    %esi
    3d68:	5f                   	pop    %edi
    3d69:	5d                   	pop    %ebp
    3d6a:	c3                   	ret
    3d6b:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    3d6f:	90                   	nop
    3d70:	b9 00 44 10 00       	mov    $0x104400,%ecx
    3d75:	8d 76 00             	lea    0x0(%esi),%esi
    3d78:	8d 81 80 f3 ff ff    	lea    -0xc80(%ecx),%eax
    3d7e:	66 90                	xchg   %ax,%ax
    3d80:	8b 10                	mov    (%eax),%edx
    3d82:	83 c0 04             	add    $0x4,%eax
    3d85:	89 90 fc 9b ff ff    	mov    %edx,-0x6404(%eax)
    3d8b:	39 c8                	cmp    %ecx,%eax
    3d8d:	75 f1                	jne    0x3d80
    3d8f:	8d 88 80 0c 00 00    	lea    0xc80(%eax),%ecx
    3d95:	3d 80 fc 1d 00       	cmp    $0x1dfc80,%eax
    3d9a:	75 dc                	jne    0x3d78
    3d9c:	ba 00 a5 1d 00       	mov    $0x1da500,%edx
    3da1:	8d 82 80 f3 ff ff    	lea    -0xc80(%edx),%eax
    3da7:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    3dae:	66 90                	xchg   %ax,%ax
    3db0:	c7 00 33 00 00 00    	movl   $0x33,(%eax)
    3db6:	83 c0 08             	add    $0x8,%eax
    3db9:	c7 40 fc 33 00 00 00 	movl   $0x33,-0x4(%eax)
    3dc0:	39 d0                	cmp    %edx,%eax
    3dc2:	75 ec                	jne    0x3db0
    3dc4:	8d 90 80 0c 00 00    	lea    0xc80(%eax),%edx
    3dca:	3d 80 fc 1d 00       	cmp    $0x1dfc80,%eax
    3dcf:	75 d0                	jne    0x3da1
    3dd1:	a1 60 59 00 00       	mov    0x5960,%eax
    3dd6:	3d 48 02 00 00       	cmp    $0x248,%eax
    3ddb:	7e 08                	jle    0x3de5
    3ddd:	83 e8 08             	sub    $0x8,%eax
    3de0:	a3 60 59 00 00       	mov    %eax,0x5960
    3de5:	c3                   	ret
    3de6:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    3ded:	8d 76 00             	lea    0x0(%esi),%esi
    3df0:	55                   	push   %ebp
    3df1:	89 e5                	mov    %esp,%ebp
    3df3:	56                   	push   %esi
    3df4:	53                   	push   %ebx
    3df5:	8b 75 08             	mov    0x8(%ebp),%esi
    3df8:	8b 5d 0c             	mov    0xc(%ebp),%ebx
    3dfb:	0f be 16             	movsbl (%esi),%edx
    3dfe:	83 c6 01             	add    $0x1,%esi
    3e01:	84 d2                	test   %dl,%dl
    3e03:	75 2e                	jne    0x3e33
    3e05:	eb 76                	jmp    0x3e7d
    3e07:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    3e0e:	66 90                	xchg   %ax,%ax
    3e10:	c7 05 60 b0 00 00 00 	movl   $0x0,0xb060
    3e17:	00 00 00 
    3e1a:	83 c0 0a             	add    $0xa,%eax
    3e1d:	a3 60 59 00 00       	mov    %eax,0x5960
    3e22:	3d 4d 02 00 00       	cmp    $0x24d,%eax
    3e27:	7f 45                	jg     0x3e6e
    3e29:	0f be 16             	movsbl (%esi),%edx
    3e2c:	83 c6 01             	add    $0x1,%esi
    3e2f:	84 d2                	test   %dl,%dl
    3e31:	74 4a                	je     0x3e7d
    3e33:	a1 60 59 00 00       	mov    0x5960,%eax
    3e38:	80 fa 0a             	cmp    $0xa,%dl
    3e3b:	74 d3                	je     0x3e10
    3e3d:	53                   	push   %ebx
    3e3e:	50                   	push   %eax
    3e3f:	ff 35 60 b0 00 00    	push   0xb060
    3e45:	52                   	push   %edx
    3e46:	e8 85 fe ff ff       	call   0x3cd0
    3e4b:	a1 60 b0 00 00       	mov    0xb060,%eax
    3e50:	83 c4 10             	add    $0x10,%esp
    3e53:	83 c0 08             	add    $0x8,%eax
    3e56:	a3 60 b0 00 00       	mov    %eax,0xb060
    3e5b:	3d 18 03 00 00       	cmp    $0x318,%eax
    3e60:	7f 26                	jg     0x3e88
    3e62:	a1 60 59 00 00       	mov    0x5960,%eax
    3e67:	3d 4d 02 00 00       	cmp    $0x24d,%eax
    3e6c:	7e bb                	jle    0x3e29
    3e6e:	e8 fd fe ff ff       	call   0x3d70
    3e73:	0f be 16             	movsbl (%esi),%edx
    3e76:	83 c6 01             	add    $0x1,%esi
    3e79:	84 d2                	test   %dl,%dl
    3e7b:	75 b6                	jne    0x3e33
    3e7d:	8d 65 f8             	lea    -0x8(%ebp),%esp
    3e80:	5b                   	pop    %ebx
    3e81:	5e                   	pop    %esi
    3e82:	5d                   	pop    %ebp
    3e83:	c3                   	ret
    3e84:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    3e88:	a1 60 59 00 00       	mov    0x5960,%eax
    3e8d:	c7 05 60 b0 00 00 00 	movl   $0x0,0xb060
    3e94:	00 00 00 
    3e97:	83 c0 0a             	add    $0xa,%eax
    3e9a:	a3 60 59 00 00       	mov    %eax,0x5960
    3e9f:	eb 81                	jmp    0x3e22
    3ea1:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    3ea8:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    3eaf:	90                   	nop
    3eb0:	55                   	push   %ebp
    3eb1:	b8 30 78 00 00       	mov    $0x7830,%eax
    3eb6:	89 e5                	mov    %esp,%ebp
    3eb8:	57                   	push   %edi
    3eb9:	56                   	push   %esi
    3eba:	8d 7d dc             	lea    -0x24(%ebp),%edi
    3ebd:	53                   	push   %ebx
    3ebe:	8d 5d dd             	lea    -0x23(%ebp),%ebx
    3ec1:	83 ec 1c             	sub    $0x1c,%esp
    3ec4:	8b 75 0c             	mov    0xc(%ebp),%esi
    3ec7:	8b 55 08             	mov    0x8(%ebp),%edx
    3eca:	66 89 45 dc          	mov    %ax,-0x24(%ebp)
    3ece:	8d 44 35 dd          	lea    -0x23(%ebp,%esi,1),%eax
    3ed2:	85 f6                	test   %esi,%esi
    3ed4:	7e 23                	jle    0x3ef9
    3ed6:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    3edd:	8d 76 00             	lea    0x0(%esi),%esi
    3ee0:	89 d1                	mov    %edx,%ecx
    3ee2:	83 e8 01             	sub    $0x1,%eax
    3ee5:	c1 ea 04             	shr    $0x4,%edx
    3ee8:	83 e1 0f             	and    $0xf,%ecx
    3eeb:	0f b6 89 97 58 00 00 	movzbl 0x5897(%ecx),%ecx
    3ef2:	88 48 01             	mov    %cl,0x1(%eax)
    3ef5:	39 d8                	cmp    %ebx,%eax
    3ef7:	75 e7                	jne    0x3ee0
    3ef9:	83 ec 08             	sub    $0x8,%esp
    3efc:	c6 44 35 de 00       	movb   $0x0,-0x22(%ebp,%esi,1)
    3f01:	ff 75 10             	push   0x10(%ebp)
    3f04:	57                   	push   %edi
    3f05:	e8 e6 fe ff ff       	call   0x3df0
    3f0a:	83 c4 10             	add    $0x10,%esp
    3f0d:	8d 65 f4             	lea    -0xc(%ebp),%esp
    3f10:	5b                   	pop    %ebx
    3f11:	5e                   	pop    %esi
    3f12:	5f                   	pop    %edi
    3f13:	5d                   	pop    %ebp
    3f14:	c3                   	ret
    3f15:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    3f1c:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    3f20:	55                   	push   %ebp
    3f21:	89 e5                	mov    %esp,%ebp
    3f23:	57                   	push   %edi
    3f24:	56                   	push   %esi
    3f25:	53                   	push   %ebx
    3f26:	83 ec 2c             	sub    $0x2c,%esp
    3f29:	8b 45 08             	mov    0x8(%ebp),%eax
    3f2c:	8b 55 0c             	mov    0xc(%ebp),%edx
    3f2f:	85 c0                	test   %eax,%eax
    3f31:	0f 84 99 00 00 00    	je     0x3fd0
    3f37:	89 c1                	mov    %eax,%ecx
    3f39:	89 45 d4             	mov    %eax,-0x2c(%ebp)
    3f3c:	be 67 66 66 66       	mov    $0x66666667,%esi
    3f41:	89 55 d0             	mov    %edx,-0x30(%ebp)
    3f44:	f7 d9                	neg    %ecx
    3f46:	0f 48 c8             	cmovs  %eax,%ecx
    3f49:	31 ff                	xor    %edi,%edi
    3f4b:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    3f4f:	90                   	nop
    3f50:	89 c8                	mov    %ecx,%eax
    3f52:	89 fb                	mov    %edi,%ebx
    3f54:	f7 ee                	imul   %esi
    3f56:	89 c8                	mov    %ecx,%eax
    3f58:	8d 7f 01             	lea    0x1(%edi),%edi
    3f5b:	c1 f8 1f             	sar    $0x1f,%eax
    3f5e:	c1 fa 02             	sar    $0x2,%edx
    3f61:	29 c2                	sub    %eax,%edx
    3f63:	8d 04 92             	lea    (%edx,%edx,4),%eax
    3f66:	01 c0                	add    %eax,%eax
    3f68:	29 c1                	sub    %eax,%ecx
    3f6a:	83 c1 30             	add    $0x30,%ecx
    3f6d:	88 4c 3d db          	mov    %cl,-0x25(%ebp,%edi,1)
    3f71:	89 d1                	mov    %edx,%ecx
    3f73:	85 d2                	test   %edx,%edx
    3f75:	75 d9                	jne    0x3f50
    3f77:	8b 45 d4             	mov    -0x2c(%ebp),%eax
    3f7a:	8b 55 d0             	mov    -0x30(%ebp),%edx
    3f7d:	85 c0                	test   %eax,%eax
    3f7f:	78 3f                	js     0x3fc0
    3f81:	c6 44 3d dc 00       	movb   $0x0,-0x24(%ebp,%edi,1)
    3f86:	85 db                	test   %ebx,%ebx
    3f88:	74 66                	je     0x3ff0
    3f8a:	8d 75 dc             	lea    -0x24(%ebp),%esi
    3f8d:	89 d7                	mov    %edx,%edi
    3f8f:	90                   	nop
    3f90:	0f b6 14 0e          	movzbl (%esi,%ecx,1),%edx
    3f94:	0f b6 04 1e          	movzbl (%esi,%ebx,1),%eax
    3f98:	88 04 0e             	mov    %al,(%esi,%ecx,1)
    3f9b:	83 c1 01             	add    $0x1,%ecx
    3f9e:	88 14 1e             	mov    %dl,(%esi,%ebx,1)
    3fa1:	83 eb 01             	sub    $0x1,%ebx
    3fa4:	39 d9                	cmp    %ebx,%ecx
    3fa6:	7c e8                	jl     0x3f90
    3fa8:	89 fa                	mov    %edi,%edx
    3faa:	83 ec 08             	sub    $0x8,%esp
    3fad:	52                   	push   %edx
    3fae:	56                   	push   %esi
    3faf:	e8 3c fe ff ff       	call   0x3df0
    3fb4:	83 c4 10             	add    $0x10,%esp
    3fb7:	8d 65 f4             	lea    -0xc(%ebp),%esp
    3fba:	5b                   	pop    %ebx
    3fbb:	5e                   	pop    %esi
    3fbc:	5f                   	pop    %edi
    3fbd:	5d                   	pop    %ebp
    3fbe:	c3                   	ret
    3fbf:	90                   	nop
    3fc0:	c6 44 3d dc 2d       	movb   $0x2d,-0x24(%ebp,%edi,1)
    3fc5:	c6 44 1d de 00       	movb   $0x0,-0x22(%ebp,%ebx,1)
    3fca:	89 fb                	mov    %edi,%ebx
    3fcc:	eb bc                	jmp    0x3f8a
    3fce:	66 90                	xchg   %ax,%ax
    3fd0:	89 55 0c             	mov    %edx,0xc(%ebp)
    3fd3:	c7 45 08 a8 58 00 00 	movl   $0x58a8,0x8(%ebp)
    3fda:	8d 65 f4             	lea    -0xc(%ebp),%esp
    3fdd:	5b                   	pop    %ebx
    3fde:	5e                   	pop    %esi
    3fdf:	5f                   	pop    %edi
    3fe0:	5d                   	pop    %ebp
    3fe1:	e9 0a fe ff ff       	jmp    0x3df0
    3fe6:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    3fed:	8d 76 00             	lea    0x0(%esi),%esi
    3ff0:	8d 75 dc             	lea    -0x24(%ebp),%esi
    3ff3:	eb b5                	jmp    0x3faa
    3ff5:	66 90                	xchg   %ax,%ax
    3ff7:	66 90                	xchg   %ax,%ax
    3ff9:	66 90                	xchg   %ax,%ax
    3ffb:	66 90                	xchg   %ax,%ax
    3ffd:	66 90                	xchg   %ax,%ax
    3fff:	90                   	nop
    4000:	55                   	push   %ebp
    4001:	89 e5                	mov    %esp,%ebp
    4003:	0f b6 45 08          	movzbl 0x8(%ebp),%eax
    4007:	8b 55 14             	mov    0x14(%ebp),%edx
    400a:	c1 e0 10             	shl    $0x10,%eax
    400d:	81 e2 fc 00 00 00    	and    $0xfc,%edx
    4013:	09 d0                	or     %edx,%eax
    4015:	0f b6 55 10          	movzbl 0x10(%ebp),%edx
    4019:	c1 e2 08             	shl    $0x8,%edx
    401c:	09 d0                	or     %edx,%eax
    401e:	0f b6 55 0c          	movzbl 0xc(%ebp),%edx
    4022:	c1 e2 0b             	shl    $0xb,%edx
    4025:	09 d0                	or     %edx,%eax
    4027:	ba f8 0c 00 00       	mov    $0xcf8,%edx
    402c:	0d 00 00 00 80       	or     $0x80000000,%eax
    4031:	ef                   	out    %eax,(%dx)
    4032:	ba fc 0c 00 00       	mov    $0xcfc,%edx
    4037:	ed                   	in     (%dx),%eax
    4038:	5d                   	pop    %ebp
    4039:	c3                   	ret
    403a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    4040:	55                   	push   %ebp
    4041:	89 e5                	mov    %esp,%ebp
    4043:	57                   	push   %edi
    4044:	56                   	push   %esi
    4045:	53                   	push   %ebx
    4046:	83 ec 08             	sub    $0x8,%esp
    4049:	0f b6 45 08          	movzbl 0x8(%ebp),%eax
    404d:	0f b6 55 10          	movzbl 0x10(%ebp),%edx
    4051:	c1 e0 10             	shl    $0x10,%eax
    4054:	c1 e2 08             	shl    $0x8,%edx
    4057:	89 c7                	mov    %eax,%edi
    4059:	0f b6 45 0c          	movzbl 0xc(%ebp),%eax
    405d:	89 c1                	mov    %eax,%ecx
    405f:	89 45 ec             	mov    %eax,-0x14(%ebp)
    4062:	c1 e1 0b             	shl    $0xb,%ecx
    4065:	09 f9                	or     %edi,%ecx
    4067:	09 d1                	or     %edx,%ecx
    4069:	89 ce                	mov    %ecx,%esi
    406b:	81 ce 00 00 00 80    	or     $0x80000000,%esi
    4071:	89 f0                	mov    %esi,%eax
    4073:	be f8 0c 00 00       	mov    $0xcf8,%esi
    4078:	89 f2                	mov    %esi,%edx
    407a:	ef                   	out    %eax,(%dx)
    407b:	bb fc 0c 00 00       	mov    $0xcfc,%ebx
    4080:	89 da                	mov    %ebx,%edx
    4082:	ed                   	in     (%dx),%eax
    4083:	89 45 f0             	mov    %eax,-0x10(%ebp)
    4086:	66 83 7d f0 ff       	cmpw   $0xffff,-0x10(%ebp)
    408b:	74 50                	je     0x40dd
    408d:	89 ca                	mov    %ecx,%edx
    408f:	81 ca 08 00 00 80    	or     $0x80000008,%edx
    4095:	89 d0                	mov    %edx,%eax
    4097:	89 f2                	mov    %esi,%edx
    4099:	ef                   	out    %eax,(%dx)
    409a:	89 da                	mov    %ebx,%edx
    409c:	ed                   	in     (%dx),%eax
    409d:	8b 0d 80 fc 1d 00    	mov    0x1dfc80,%ecx
    40a3:	83 f9 0f             	cmp    $0xf,%ecx
    40a6:	7f 35                	jg     0x40dd
    40a8:	8b 5d f0             	mov    -0x10(%ebp),%ebx
    40ab:	31 d2                	xor    %edx,%edx
    40ad:	89 1c cd a0 fc 1d 00 	mov    %ebx,0x1dfca0(,%ecx,8)
    40b4:	89 c3                	mov    %eax,%ebx
    40b6:	c1 e8 10             	shr    $0x10,%eax
    40b9:	c1 eb 18             	shr    $0x18,%ebx
    40bc:	88 da                	mov    %bl,%dl
    40be:	88 c6                	mov    %al,%dh
    40c0:	8b 45 ec             	mov    -0x14(%ebp),%eax
    40c3:	0f b7 d2             	movzwl %dx,%edx
    40c6:	c1 e0 18             	shl    $0x18,%eax
    40c9:	09 fa                	or     %edi,%edx
    40cb:	09 c2                	or     %eax,%edx
    40cd:	89 14 cd a4 fc 1d 00 	mov    %edx,0x1dfca4(,%ecx,8)
    40d4:	83 c1 01             	add    $0x1,%ecx
    40d7:	89 0d 80 fc 1d 00    	mov    %ecx,0x1dfc80
    40dd:	83 c4 08             	add    $0x8,%esp
    40e0:	5b                   	pop    %ebx
    40e1:	5e                   	pop    %esi
    40e2:	5f                   	pop    %edi
    40e3:	5d                   	pop    %ebp
    40e4:	c3                   	ret
    40e5:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    40ec:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    40f0:	55                   	push   %ebp
    40f1:	31 c9                	xor    %ecx,%ecx
    40f3:	89 e5                	mov    %esp,%ebp
    40f5:	57                   	push   %edi
    40f6:	31 ff                	xor    %edi,%edi
    40f8:	56                   	push   %esi
    40f9:	53                   	push   %ebx
    40fa:	83 ec 10             	sub    $0x10,%esp
    40fd:	c6 45 eb 00          	movb   $0x0,-0x15(%ebp)
    4101:	c7 05 80 fc 1d 00 00 	movl   $0x0,0x1dfc80
    4108:	00 00 00 
    410b:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
    410f:	90                   	nop
    4110:	89 c8                	mov    %ecx,%eax
    4112:	0f b6 d1             	movzbl %cl,%edx
    4115:	89 4d e4             	mov    %ecx,-0x1c(%ebp)
    4118:	31 db                	xor    %ebx,%ebx
    411a:	c1 e0 10             	shl    $0x10,%eax
    411d:	c1 e2 10             	shl    $0x10,%edx
    4120:	89 45 f0             	mov    %eax,-0x10(%ebp)
    4123:	89 55 ec             	mov    %edx,-0x14(%ebp)
    4126:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    412d:	8d 76 00             	lea    0x0(%esi),%esi
    4130:	8b 45 f0             	mov    -0x10(%ebp),%eax
    4133:	89 d9                	mov    %ebx,%ecx
    4135:	ba f8 0c 00 00       	mov    $0xcf8,%edx
    413a:	c1 e1 0b             	shl    $0xb,%ecx
    413d:	09 c1                	or     %eax,%ecx
    413f:	89 c8                	mov    %ecx,%eax
    4141:	0d 00 00 00 80       	or     $0x80000000,%eax
    4146:	ef                   	out    %eax,(%dx)
    4147:	ba fc 0c 00 00       	mov    $0xcfc,%edx
    414c:	ed                   	in     (%dx),%eax
    414d:	89 c6                	mov    %eax,%esi
    414f:	66 83 f8 ff          	cmp    $0xffff,%ax
    4153:	74 4a                	je     0x419f
    4155:	89 c8                	mov    %ecx,%eax
    4157:	ba f8 0c 00 00       	mov    $0xcf8,%edx
    415c:	0d 08 00 00 80       	or     $0x80000008,%eax
    4161:	ef                   	out    %eax,(%dx)
    4162:	ba fc 0c 00 00       	mov    $0xcfc,%edx
    4167:	ed                   	in     (%dx),%eax
    4168:	83 ff 0f             	cmp    $0xf,%edi
    416b:	7f 32                	jg     0x419f
    416d:	89 c1                	mov    %eax,%ecx
    416f:	31 d2                	xor    %edx,%edx
    4171:	c1 e8 10             	shr    $0x10,%eax
    4174:	89 34 fd a0 fc 1d 00 	mov    %esi,0x1dfca0(,%edi,8)
    417b:	c1 e9 18             	shr    $0x18,%ecx
    417e:	8b 75 ec             	mov    -0x14(%ebp),%esi
    4181:	c6 45 eb 01          	movb   $0x1,-0x15(%ebp)
    4185:	88 ca                	mov    %cl,%dl
    4187:	88 c6                	mov    %al,%dh
    4189:	89 d8                	mov    %ebx,%eax
    418b:	0f b7 d2             	movzwl %dx,%edx
    418e:	c1 e0 18             	shl    $0x18,%eax
    4191:	09 f2                	or     %esi,%edx
    4193:	09 c2                	or     %eax,%edx
    4195:	89 14 fd a4 fc 1d 00 	mov    %edx,0x1dfca4(,%edi,8)
    419c:	83 c7 01             	add    $0x1,%edi
    419f:	83 c3 01             	add    $0x1,%ebx
    41a2:	83 fb 20             	cmp    $0x20,%ebx
    41a5:	75 89                	jne    0x4130
    41a7:	8b 4d e4             	mov    -0x1c(%ebp),%ecx
    41aa:	83 c1 01             	add    $0x1,%ecx
    41ad:	81 f9 00 01 00 00    	cmp    $0x100,%ecx
    41b3:	0f 85 57 ff ff ff    	jne    0x4110
    41b9:	80 7d eb 00          	cmpb   $0x0,-0x15(%ebp)
    41bd:	74 06                	je     0x41c5
    41bf:	89 3d 80 fc 1d 00    	mov    %edi,0x1dfc80
    41c5:	83 c4 10             	add    $0x10,%esp
    41c8:	5b                   	pop    %ebx
    41c9:	5e                   	pop    %esi
    41ca:	5f                   	pop    %edi
    41cb:	5d                   	pop    %ebp
    41cc:	c3                   	ret
    41cd:	8d 76 00             	lea    0x0(%esi),%esi
    41d0:	55                   	push   %ebp
    41d1:	89 e5                	mov    %esp,%ebp
    41d3:	53                   	push   %ebx
    41d4:	83 ec 0c             	sub    $0xc,%esp
    41d7:	68 ff ff 00 00       	push   $0xffff
    41dc:	68 aa 58 00 00       	push   $0x58aa
    41e1:	e8 0a fc ff ff       	call   0x3df0
    41e6:	a1 80 fc 1d 00       	mov    0x1dfc80,%eax
    41eb:	83 c4 10             	add    $0x10,%esp
    41ee:	85 c0                	test   %eax,%eax
    41f0:	0f 8e d2 01 00 00    	jle    0x43c8
    41f6:	31 db                	xor    %ebx,%ebx
    41f8:	eb 50                	jmp    0x424a
    41fa:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    4200:	3c 02                	cmp    $0x2,%al
    4202:	0f 84 80 01 00 00    	je     0x4388
    4208:	3c 03                	cmp    $0x3,%al
    420a:	0f 84 98 01 00 00    	je     0x43a8
    4210:	3c 0c                	cmp    $0xc,%al
    4212:	75 14                	jne    0x4228
    4214:	80 3c dd a5 fc 1d 00 	cmpb   $0x3,0x1dfca5(,%ebx,8)
    421b:	03 
    421c:	0f 84 f6 01 00 00    	je     0x4418
    4222:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    4228:	83 ec 08             	sub    $0x8,%esp
    422b:	83 c3 01             	add    $0x1,%ebx
    422e:	6a 00                	push   $0x0
    4230:	68 ff 58 00 00       	push   $0x58ff
    4235:	e8 b6 fb ff ff       	call   0x3df0
    423a:	a1 80 fc 1d 00       	mov    0x1dfc80,%eax
    423f:	83 c4 10             	add    $0x10,%esp
    4242:	39 d8                	cmp    %ebx,%eax
    4244:	0f 8e 7e 01 00 00    	jle    0x43c8
    424a:	83 ec 08             	sub    $0x8,%esp
    424d:	68 ff ff 00 00       	push   $0xffff
    4252:	68 bf 58 00 00       	push   $0x58bf
    4257:	e8 94 fb ff ff       	call   0x3df0
    425c:	0f b6 04 dd a6 fc 1d 	movzbl 0x1dfca6(,%ebx,8),%eax
    4263:	00 
    4264:	83 c4 0c             	add    $0xc,%esp
    4267:	68 ff ff ff 00       	push   $0xffffff
    426c:	6a 02                	push   $0x2
    426e:	50                   	push   %eax
    426f:	e8 3c fc ff ff       	call   0x3eb0
    4274:	58                   	pop    %eax
    4275:	5a                   	pop    %edx
    4276:	68 ff ff 00 00       	push   $0xffff
    427b:	68 c2 58 00 00       	push   $0x58c2
    4280:	e8 6b fb ff ff       	call   0x3df0
    4285:	0f b6 04 dd a7 fc 1d 	movzbl 0x1dfca7(,%ebx,8),%eax
    428c:	00 
    428d:	83 c4 0c             	add    $0xc,%esp
    4290:	68 ff ff ff 00       	push   $0xffffff
    4295:	6a 02                	push   $0x2
    4297:	50                   	push   %eax
    4298:	e8 13 fc ff ff       	call   0x3eb0
    429d:	59                   	pop    %ecx
    429e:	58                   	pop    %eax
    429f:	68 ff ff 00 00       	push   $0xffff
    42a4:	68 c6 58 00 00       	push   $0x58c6
    42a9:	e8 42 fb ff ff       	call   0x3df0
    42ae:	0f b7 04 dd a0 fc 1d 	movzwl 0x1dfca0(,%ebx,8),%eax
    42b5:	00 
    42b6:	83 c4 0c             	add    $0xc,%esp
    42b9:	68 00 ff ff 00       	push   $0xffff00
    42be:	6a 04                	push   $0x4
    42c0:	50                   	push   %eax
    42c1:	e8 ea fb ff ff       	call   0x3eb0
    42c6:	58                   	pop    %eax
    42c7:	5a                   	pop    %edx
    42c8:	68 ff ff 00 00       	push   $0xffff
    42cd:	68 cc 58 00 00       	push   $0x58cc
    42d2:	e8 19 fb ff ff       	call   0x3df0
    42d7:	0f b7 04 dd a2 fc 1d 	movzwl 0x1dfca2(,%ebx,8),%eax
    42de:	00 
    42df:	83 c4 0c             	add    $0xc,%esp
    42e2:	68 00 ff ff 00       	push   $0xffff00
    42e7:	6a 04                	push   $0x4
    42e9:	50                   	push   %eax
    42ea:	e8 c1 fb ff ff       	call   0x3eb0
    42ef:	59                   	pop    %ecx
    42f0:	58                   	pop    %eax
    42f1:	68 ff ff 00 00       	push   $0xffff
    42f6:	68 d2 58 00 00       	push   $0x58d2
    42fb:	e8 f0 fa ff ff       	call   0x3df0
    4300:	0f b6 04 dd a4 fc 1d 	movzbl 0x1dfca4(,%ebx,8),%eax
    4307:	00 
    4308:	83 c4 0c             	add    $0xc,%esp
    430b:	68 00 88 ff 00       	push   $0xff8800
    4310:	6a 02                	push   $0x2
    4312:	50                   	push   %eax
    4313:	e8 98 fb ff ff       	call   0x3eb0
    4318:	58                   	pop    %eax
    4319:	5a                   	pop    %edx
    431a:	68 88 88 88 00       	push   $0x888888
    431f:	68 d8 58 00 00       	push   $0x58d8
    4324:	e8 c7 fa ff ff       	call   0x3df0
    4329:	0f b6 04 dd a5 fc 1d 	movzbl 0x1dfca5(,%ebx,8),%eax
    4330:	00 
    4331:	83 c4 0c             	add    $0xc,%esp
    4334:	68 00 88 ff 00       	push   $0xff8800
    4339:	6a 02                	push   $0x2
    433b:	50                   	push   %eax
    433c:	e8 6f fb ff ff       	call   0x3eb0
    4341:	0f b6 04 dd a4 fc 1d 	movzbl 0x1dfca4(,%ebx,8),%eax
    4348:	00 
    4349:	83 c4 10             	add    $0x10,%esp
    434c:	3c 01                	cmp    $0x1,%al
    434e:	0f 85 ac fe ff ff    	jne    0x4200
    4354:	0f b6 04 dd a5 fc 1d 	movzbl 0x1dfca5(,%ebx,8),%eax
    435b:	00 
    435c:	3c 08                	cmp    $0x8,%al
    435e:	74 78                	je     0x43d8
    4360:	3c 06                	cmp    $0x6,%al
    4362:	0f 85 c0 fe ff ff    	jne    0x4228
    4368:	83 ec 08             	sub    $0x8,%esp
    436b:	68 88 ff 00 00       	push   $0xff88
    4370:	68 e2 58 00 00       	push   $0x58e2
    4375:	e8 76 fa ff ff       	call   0x3df0
    437a:	83 c4 10             	add    $0x10,%esp
    437d:	e9 a6 fe ff ff       	jmp    0x4228
    4382:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    4388:	83 ec 08             	sub    $0x8,%esp
    438b:	68 ff ff 88 00       	push   $0x88ffff
    4390:	68 ea 58 00 00       	push   $0x58ea
    4395:	e8 56 fa ff ff       	call   0x3df0
    439a:	83 c4 10             	add    $0x10,%esp
    439d:	e9 86 fe ff ff       	jmp    0x4228
    43a2:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    43a8:	83 ec 08             	sub    $0x8,%esp
    43ab:	68 ff 88 88 00       	push   $0x8888ff
    43b0:	68 f1 58 00 00       	push   $0x58f1
    43b5:	e8 36 fa ff ff       	call   0x3df0
    43ba:	83 c4 10             	add    $0x10,%esp
    43bd:	e9 66 fe ff ff       	jmp    0x4228
    43c2:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    43c8:	85 c0                	test   %eax,%eax
    43ca:	74 2c                	je     0x43f8
    43cc:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    43cf:	c9                   	leave
    43d0:	c3                   	ret
    43d1:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
    43d8:	83 ec 08             	sub    $0x8,%esp
    43db:	68 88 ff 00 00       	push   $0xff88
    43e0:	68 da 58 00 00       	push   $0x58da
    43e5:	e8 06 fa ff ff       	call   0x3df0
    43ea:	83 c4 10             	add    $0x10,%esp
    43ed:	e9 36 fe ff ff       	jmp    0x4228
    43f2:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    43f8:	83 ec 08             	sub    $0x8,%esp
    43fb:	68 44 44 ff 00       	push   $0xff4444
    4400:	68 01 59 00 00       	push   $0x5901
    4405:	e8 e6 f9 ff ff       	call   0x3df0
    440a:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    440d:	83 c4 10             	add    $0x10,%esp
    4410:	c9                   	leave
    4411:	c3                   	ret
    4412:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
    4418:	83 ec 08             	sub    $0x8,%esp
    441b:	68 ff 88 ff 00       	push   $0xff88ff
    4420:	68 f8 58 00 00       	push   $0x58f8
    4425:	e8 c6 f9 ff ff       	call   0x3df0
    442a:	83 c4 10             	add    $0x10,%esp
    442d:	e9 f6 fd ff ff       	jmp    0x4228
    4432:	a1 00 00 00 00       	mov    0x0,%eax
    4437:	0f 0b                	ud2
    4439:	00 00                	add    %al,(%eax)
    443b:	00 00                	add    %al,(%eax)
    443d:	00 00                	add    %al,(%eax)
    443f:	00 30                	add    %dh,(%eax)
    4441:	3f                   	aas
    4442:	00 00                	add    %al,(%eax)
    4444:	d0 3e                	sarb   (%esi)
    4446:	00 00                	add    %al,(%eax)
    4448:	10 3f                	adc    %bh,(%edi)
    444a:	00 00                	add    %al,(%eax)
    444c:	a8 3e                	test   $0x3e,%al
    444e:	00 00                	add    %al,(%eax)
    4450:	e8 3e 00 00 00       	call   0x4493
	...
    4461:	1b 31                	sbb    (%ecx),%esi
    4463:	32 33                	xor    (%ebx),%dh
    4465:	34 35                	xor    $0x35,%al
    4467:	36 37                	ss aaa
    4469:	38 39                	cmp    %bh,(%ecx)
    446b:	30 2d 3d 08 09 71    	xor    %ch,0x7109083d
    4471:	77 65                	ja     0x44d8
    4473:	72 74                	jb     0x44e9
    4475:	79 75                	jns    0x44ec
    4477:	69 6f 70 5b 5d 0a 00 	imul   $0xa5d5b,0x70(%edi),%ebp
    447e:	61                   	popa
    447f:	73 64                	jae    0x44e5
    4481:	66 67 68 6a 6b       	addr16 pushw $0x6b6a
    4486:	6c                   	insb   (%dx),%es:(%edi)
    4487:	3b 27                	cmp    (%edi),%esp
    4489:	60                   	pusha
    448a:	00 5c 7a 78          	add    %bl,0x78(%edx,%edi,2)
    448e:	63 76 62             	arpl   %si,0x62(%esi)
    4491:	6e                   	outsb  %ds:(%esi),(%dx)
    4492:	6d                   	insl   (%dx),%es:(%edi)
    4493:	2c 2e                	sub    $0x2e,%al
    4495:	2f                   	das
    4496:	00 2a                	add    %ch,(%edx)
    4498:	00 20                	add    %ah,(%eax)
	...
    44de:	00 00                	add    %al,(%eax)
    44e0:	46                   	inc    %esi
    44e1:	4c                   	dec    %esp
    44e2:	55                   	push   %ebp
    44e3:	49                   	dec    %ecx
    44e4:	44                   	inc    %esp
    44e5:	20 00                	and    %al,(%eax)
    44e7:	52                   	push   %edx
    44e8:	49                   	dec    %ecx
    44e9:	47                   	inc    %edi
    44ea:	49                   	dec    %ecx
    44eb:	44                   	inc    %esp
    44ec:	20 00                	and    %al,(%eax)
    44ee:	53                   	push   %ebx
    44ef:	50                   	push   %eax
    44f0:	53                   	push   %ebx
    44f1:	3a 20                	cmp    (%eax),%ah
    44f3:	00 54 48 52          	add    %dl,0x52(%eax,%ecx,2)
    44f7:	3a 20                	cmp    (%eax),%ah
    44f9:	00 54 4f 54          	add    %dl,0x54(%edi,%ecx,2)
    44fd:	41                   	inc    %ecx
    44fe:	4c                   	dec    %esp
    44ff:	20 47 52             	and    %al,0x52(%edi)
    4502:	49                   	dec    %ecx
    4503:	44                   	inc    %esp
    4504:	20 53 50             	and    %dl,0x50(%ebx)
    4507:	49                   	dec    %ecx
    4508:	4b                   	dec    %ebx
    4509:	45                   	inc    %ebp
    450a:	53                   	push   %ebx
    450b:	3a 20                	cmp    (%eax),%ah
    450d:	00 30                	add    %dh,(%eax)
    450f:	31 32                	xor    %esi,(%edx)
    4511:	33 34 35 36 37 38 39 	xor    0x39383736(,%esi,1),%esi
    4518:	41                   	inc    %ecx
    4519:	42                   	inc    %edx
    451a:	43                   	inc    %ebx
    451b:	44                   	inc    %esp
    451c:	45                   	inc    %ebp
    451d:	46                   	inc    %esi
    451e:	00 4e 45             	add    %cl,0x45(%esi)
    4521:	55                   	push   %ebp
    4522:	52                   	push   %edx
    4523:	4f                   	dec    %edi
    4524:	53                   	push   %ebx
    4525:	50                   	push   %eax
    4526:	41                   	inc    %ecx
    4527:	52                   	push   %edx
    4528:	4b                   	dec    %ebx
    4529:	20 4f 53             	and    %cl,0x53(%edi)
    452c:	20 00                	and    %al,(%eax)
    452e:	7c 20                	jl     0x4550
    4530:	00 50 43             	add    %dl,0x43(%eax)
    4533:	49                   	dec    %ecx
    4534:	3a 20                	cmp    (%eax),%ah
    4536:	00 20                	add    %ah,(%eax)
    4538:	64 65 76 20          	fs gs jbe 0x455c
    453c:	20 00                	and    %al,(%eax)
    453e:	5b                   	pop    %ebx
    453f:	4e                   	dec    %esi
    4540:	56                   	push   %esi
    4541:	4d                   	dec    %ebp
    4542:	65 40                	gs inc %eax
    4544:	42                   	inc    %edx
    4545:	00 5d 20             	add    %bl,0x20(%ebp)
    4548:	00 5b 41             	add    %bl,0x41(%ebx)
    454b:	48                   	dec    %eax
    454c:	43                   	inc    %ebx
    454d:	49                   	dec    %ecx
    454e:	40                   	inc    %eax
    454f:	42                   	inc    %edx
    4550:	00 4d 45             	add    %cl,0x45(%ebp)
    4553:	4d                   	dec    %ebp
    4554:	20 46 52             	and    %al,0x52(%esi)
    4557:	45                   	inc    %ebp
    4558:	45                   	inc    %ebp
    4559:	3a 20                	cmp    (%eax),%ah
    455b:	00 20                	add    %ah,(%eax)
    455d:	70 61                	jo     0x45c0
    455f:	67 65 73 20          	addr16 gs jae 0x4583
    4563:	28 00                	sub    %al,(%eax)
    4565:	20 4b 42             	and    %cl,0x42(%ebx)
    4568:	29 00                	sub    %eax,(%eax)
    456a:	4e                   	dec    %esi
    456b:	45                   	inc    %ebp
    456c:	55                   	push   %ebp
    456d:	52                   	push   %edx
    456e:	4f                   	dec    %edi
    456f:	4e                   	dec    %esi
    4570:	53                   	push   %ebx
    4571:	3a 20                	cmp    (%eax),%ah
    4573:	00 20                	add    %ah,(%eax)
    4575:	20 54 49 43          	and    %dl,0x43(%ecx,%ecx,2)
    4579:	4b                   	dec    %ebx
    457a:	3a 20                	cmp    (%eax),%ah
    457c:	00 20                	add    %ah,(%eax)
    457e:	20 53 50             	and    %dl,0x50(%ebx)
    4581:	53                   	push   %ebx
    4582:	5b                   	pop    %ebx
    4583:	30 5d 3a             	xor    %bl,0x3a(%ebp)
    4586:	20 00                	and    %al,(%eax)
    4588:	20 20                	and    %ah,(%eax)
    458a:	53                   	push   %ebx
    458b:	50                   	push   %eax
    458c:	53                   	push   %ebx
    458d:	5b                   	pop    %ebx
    458e:	31 5d 3a             	xor    %ebx,0x3a(%ebp)
    4591:	20 00                	and    %al,(%eax)
    4593:	4e                   	dec    %esi
    4594:	45                   	inc    %ebp
    4595:	55                   	push   %ebp
    4596:	52                   	push   %edx
    4597:	41                   	inc    %ecx
    4598:	4c                   	dec    %esp
    4599:	20 57 41             	and    %dl,0x41(%edi)
    459c:	56                   	push   %esi
    459d:	45                   	inc    %ebp
    459e:	46                   	inc    %esi
    459f:	4f                   	dec    %edi
    45a0:	52                   	push   %edx
    45a1:	4d                   	dec    %ebp
    45a2:	20 5b 53             	and    %bl,0x53(%ebx)
    45a5:	52                   	push   %edx
    45a6:	4e                   	dec    %esi
    45a7:	45                   	inc    %ebp
    45a8:	4d                   	dec    %ebp
    45a9:	5d                   	pop    %ebp
    45aa:	00 4e 41             	add    %cl,0x41(%esi)
    45ad:	4d                   	dec    %ebp
    45ae:	45                   	inc    %ebp
    45af:	20 20                	and    %ah,(%eax)
    45b1:	20 20                	and    %ah,(%eax)
    45b3:	20 20                	and    %ah,(%eax)
    45b5:	20 20                	and    %ah,(%eax)
    45b7:	20 4c 42 41          	and    %cl,0x41(%edx,%eax,2)
    45bb:	20 20                	and    %ah,(%eax)
    45bd:	20 20                	and    %ah,(%eax)
    45bf:	53                   	push   %ebx
    45c0:	49                   	dec    %ecx
    45c1:	5a                   	pop    %edx
    45c2:	45                   	inc    %ebp
    45c3:	00 2d 2d 2d 2d 2d    	add    %ch,0x2d2d2d2d
    45c9:	2d 2d 2d 2d 2d       	sub    $0x2d2d2d2d,%eax
    45ce:	2d 2d 2d 2d 2d       	sub    $0x2d2d2d2d,%eax
    45d3:	2d 2d 2d 2d 2d       	sub    $0x2d2d2d2d,%eax
    45d8:	2d 2d 2d 2d 00       	sub    $0x2d2d2d,%eax
    45dd:	4e                   	dec    %esi
    45de:	4f                   	dec    %edi
    45df:	20 46 49             	and    %al,0x49(%esi)
    45e2:	4c                   	dec    %esp
    45e3:	45                   	inc    %ebp
    45e4:	53                   	push   %ebx
    45e5:	20 46 4f             	and    %al,0x4f(%esi)
    45e8:	55                   	push   %ebp
    45e9:	4e                   	dec    %esi
    45ea:	44                   	inc    %esp
    45eb:	2e 00 49 4f          	add    %cl,%cs:0x4f(%ecx)
    45ef:	20 20                	and    %ah,(%eax)
    45f1:	00 43 4f             	add    %al,0x4f(%ebx)
    45f4:	4d                   	dec    %ebp
    45f5:	50                   	push   %eax
    45f6:	00 4d 55             	add    %cl,0x55(%ebp)
    45f9:	4c                   	dec    %esp
    45fa:	54                   	push   %esp
    45fb:	49                   	dec    %ecx
    45fc:	54                   	push   %esp
    45fd:	41                   	inc    %ecx
    45fe:	53                   	push   %ebx
    45ff:	4b                   	dec    %ebx
    4600:	49                   	dec    %ecx
    4601:	4e                   	dec    %esi
    4602:	47                   	inc    %edi
    4603:	20 4b 45             	and    %cl,0x45(%ebx)
    4606:	52                   	push   %edx
    4607:	4e                   	dec    %esi
    4608:	45                   	inc    %ebp
    4609:	4c                   	dec    %esp
    460a:	20 41 43             	and    %al,0x43(%ecx)
    460d:	54                   	push   %esp
    460e:	49                   	dec    %ecx
    460f:	56                   	push   %esi
    4610:	45                   	inc    %ebp
    4611:	00 2d 2d 2d 20 4e    	add    %ch,0x4e202d2d
    4617:	45                   	inc    %ebp
    4618:	55                   	push   %ebp
    4619:	52                   	push   %edx
    461a:	4f                   	dec    %edi
    461b:	2d 46 49 4c 45       	sub    $0x454c4946,%eax
    4620:	53                   	push   %ebx
    4621:	59                   	pop    %ecx
    4622:	53                   	push   %ebx
    4623:	54                   	push   %esp
    4624:	45                   	inc    %ebp
    4625:	4d                   	dec    %ebp
    4626:	20 2d 2d 2d 0a 00    	and    %ch,0xa2d2d
    462c:	0a 00                	or     (%eax),%al
    462e:	4e                   	dec    %esi
    462f:	4f                   	dec    %edi
    4630:	20 46 49             	and    %al,0x49(%esi)
    4633:	4c                   	dec    %esp
    4634:	45                   	inc    %ebp
    4635:	53                   	push   %ebx
    4636:	20 46 4f             	and    %al,0x4f(%esi)
    4639:	55                   	push   %ebp
    463a:	4e                   	dec    %esi
    463b:	44                   	inc    %esp
    463c:	2e 0a 00             	or     %cs:(%eax),%al
    463f:	53                   	push   %ebx
    4640:	43                   	inc    %ebx
    4641:	41                   	inc    %ecx
    4642:	4e                   	dec    %esi
    4643:	4e                   	dec    %esi
    4644:	49                   	dec    %ecx
    4645:	4e                   	dec    %esi
    4646:	47                   	inc    %edi
    4647:	20 4e 45             	and    %cl,0x45(%esi)
    464a:	55                   	push   %ebp
    464b:	52                   	push   %edx
    464c:	4f                   	dec    %edi
    464d:	2d 46 49 4c 45       	sub    $0x454c4946,%eax
    4652:	53                   	push   %ebx
    4653:	59                   	pop    %ecx
    4654:	53                   	push   %ebx
    4655:	54                   	push   %esp
    4656:	45                   	inc    %ebp
    4657:	4d                   	dec    %ebp
    4658:	2e 2e 2e 0a 00       	cs cs or %cs:(%eax),%al
    465d:	73 61                	jae    0x46c0
    465f:	76 65                	jbe    0x46c6
    4661:	20 30                	and    %dh,(%eax)
    4663:	2c 20                	sub    $0x20,%al
    4665:	6c                   	insb   (%dx),%es:(%edi)
    4666:	6f                   	outsl  %ds:(%esi),(%dx)
    4667:	61                   	popa
    4668:	64 20 30             	and    %dh,%fs:(%eax)
    466b:	2c 20                	sub    $0x20,%al
    466d:	73 74                	jae    0x46e3
    466f:	69 6d 2c 20 65 76 61 	imul   $0x61766520,0x2c(%ebp),%ebp
    4676:	6c                   	insb   (%dx),%es:(%edi)
    4677:	00 6d 61             	add    %ch,0x61(%ebp)
    467a:	6c                   	insb   (%dx),%es:(%edi)
    467b:	6c                   	insb   (%dx),%es:(%edi)
    467c:	2c 20                	sub    $0x20,%al
    467e:	66 72 65             	data16 jb 0x46e6
    4681:	65 2c 20             	gs sub $0x20,%al
    4684:	6d                   	insl   (%dx),%es:(%edi)
    4685:	61                   	popa
    4686:	70 2c                	jo     0x46b4
    4688:	20 77 69             	and    %dh,0x69(%edi)
    468b:	70 65                	jo     0x46f2
    468d:	2c 20                	sub    $0x20,%al
    468f:	63 6c 65 61          	arpl   %bp,0x61(%ebp,%eiz,2)
    4693:	72 00                	jb     0x4695
    4695:	4e                   	dec    %esi
    4696:	45                   	inc    %ebp
    4697:	55                   	push   %ebp
    4698:	52                   	push   %edx
    4699:	41                   	inc    %ecx
    469a:	4c                   	dec    %esp
    469b:	20 45 56             	and    %al,0x56(%ebp)
    469e:	41                   	inc    %ecx
    469f:	4c                   	dec    %esp
    46a0:	55                   	push   %ebp
    46a1:	41                   	inc    %ecx
    46a2:	54                   	push   %esp
    46a3:	49                   	dec    %ecx
    46a4:	4f                   	dec    %edi
    46a5:	4e                   	dec    %esi
    46a6:	3a 00                	cmp    (%eax),%al
    46a8:	3e 3e 20 46 4c       	ds and %al,%ds:0x4c(%esi)
    46ad:	55                   	push   %ebp
    46ae:	49                   	dec    %ecx
    46af:	44                   	inc    %esp
    46b0:	2f                   	das
    46b1:	41                   	inc    %ecx
    46b2:	43                   	inc    %ebx
    46b3:	54                   	push   %esp
    46b4:	49                   	dec    %ecx
    46b5:	56                   	push   %esi
    46b6:	45                   	inc    %ebp
    46b7:	20 3c 3c             	and    %bh,(%esp,%edi,1)
    46ba:	00 3e                	add    %bh,(%esi)
    46bc:	3e 20 52 49          	and    %dl,%ds:0x49(%edx)
    46c0:	47                   	inc    %edi
    46c1:	49                   	dec    %ecx
    46c2:	44                   	inc    %esp
    46c3:	2f                   	das
    46c4:	53                   	push   %ebx
    46c5:	54                   	push   %esp
    46c6:	41                   	inc    %ecx
    46c7:	42                   	inc    %edx
    46c8:	4c                   	dec    %esp
    46c9:	45                   	inc    %ebp
    46ca:	20 3c 3c             	and    %bh,(%esp,%edi,1)
    46cd:	00 53 54             	add    %dl,0x54(%ebx)
    46d0:	49                   	dec    %ecx
    46d1:	4d                   	dec    %ebp
    46d2:	55                   	push   %ebp
    46d3:	4c                   	dec    %esp
    46d4:	55                   	push   %ebp
    46d5:	53                   	push   %ebx
    46d6:	20 49 4e             	and    %cl,0x4e(%ecx)
    46d9:	4a                   	dec    %edx
    46da:	45                   	inc    %ebp
    46db:	43                   	inc    %ebx
    46dc:	54                   	push   %esp
    46dd:	45                   	inc    %ebp
    46de:	44                   	inc    %esp
    46df:	3a 20                	cmp    (%eax),%ah
    46e1:	00 45 52             	add    %al,0x52(%ebp)
    46e4:	52                   	push   %edx
    46e5:	3a 20                	cmp    (%eax),%ah
    46e7:	49                   	dec    %ecx
    46e8:	4e                   	dec    %esi
    46e9:	56                   	push   %esi
    46ea:	41                   	inc    %ecx
    46eb:	4c                   	dec    %esp
    46ec:	49                   	dec    %ecx
    46ed:	44                   	inc    %esp
    46ee:	20 54 41 53          	and    %dl,0x53(%ecx,%eax,2)
    46f2:	4b                   	dec    %ebx
    46f3:	20 49 44             	and    %cl,0x44(%ecx)
    46f6:	00 45 52             	add    %al,0x52(%ebp)
    46f9:	52                   	push   %edx
    46fa:	3a 20                	cmp    (%eax),%ah
    46fc:	4f                   	dec    %edi
    46fd:	55                   	push   %ebp
    46fe:	54                   	push   %esp
    46ff:	20 4f 46             	and    %cl,0x46(%edi)
    4702:	20 50 48             	and    %dl,0x48(%eax)
    4705:	59                   	pop    %ecx
    4706:	53                   	push   %ebx
    4707:	49                   	dec    %ecx
    4708:	43                   	inc    %ebx
    4709:	41                   	inc    %ecx
    470a:	4c                   	dec    %esp
    470b:	20 4d 45             	and    %cl,0x45(%ebp)
    470e:	4d                   	dec    %ebp
    470f:	4f                   	dec    %edi
    4710:	52                   	push   %edx
    4711:	59                   	pop    %ecx
    4712:	00 50 41             	add    %dl,0x41(%eax)
    4715:	47                   	inc    %edi
    4716:	45                   	inc    %ebp
    4717:	20 52 45             	and    %dl,0x45(%edx)
    471a:	4c                   	dec    %esp
    471b:	45                   	inc    %ebp
    471c:	41                   	inc    %ecx
    471d:	53                   	push   %ebx
    471e:	45                   	inc    %ebp
    471f:	44                   	inc    %esp
    4720:	20 41 54             	and    %al,0x54(%ecx)
    4723:	3a 20                	cmp    (%eax),%ah
    4725:	00 2d 2d 2d 20 4d    	add    %ch,0x4d202d2d
    472b:	45                   	inc    %ebp
    472c:	4d                   	dec    %ebp
    472d:	4f                   	dec    %edi
    472e:	52                   	push   %edx
    472f:	59                   	pop    %ecx
    4730:	20 4d 41             	and    %cl,0x41(%ebp)
    4733:	50                   	push   %eax
    4734:	20 2d 2d 2d 00 53    	and    %ch,0x53002d2d
    473a:	41                   	inc    %ecx
    473b:	56                   	push   %esi
    473c:	49                   	dec    %ecx
    473d:	4e                   	dec    %esi
    473e:	47                   	inc    %edi
    473f:	20 53 4e             	and    %dl,0x4e(%ebx)
    4742:	41                   	inc    %ecx
    4743:	50                   	push   %eax
    4744:	53                   	push   %ebx
    4745:	48                   	dec    %eax
    4746:	4f                   	dec    %edi
    4747:	54                   	push   %esp
    4748:	2e 2e 2e 00 53 41    	cs cs add %dl,%cs:0x41(%ebx)
    474e:	56                   	push   %esi
    474f:	45                   	inc    %ebp
    4750:	44                   	inc    %esp
    4751:	20 41 53             	and    %al,0x53(%ecx)
    4754:	3a 20                	cmp    (%eax),%ah
    4756:	00 45 52             	add    %al,0x52(%ebp)
    4759:	52                   	push   %edx
    475a:	3a 20                	cmp    (%eax),%ah
    475c:	44                   	inc    %esp
    475d:	49                   	dec    %ecx
    475e:	53                   	push   %ebx
    475f:	4b                   	dec    %ebx
    4760:	20 44 49 52          	and    %al,0x52(%ecx,%ecx,2)
    4764:	45                   	inc    %ebp
    4765:	43                   	inc    %ebx
    4766:	54                   	push   %esp
    4767:	4f                   	dec    %edi
    4768:	52                   	push   %edx
    4769:	59                   	pop    %ecx
    476a:	20 46 55             	and    %al,0x55(%esi)
    476d:	4c                   	dec    %esp
    476e:	4c                   	dec    %esp
    476f:	00 55 53             	add    %dl,0x53(%ebp)
    4772:	41                   	inc    %ecx
    4773:	47                   	inc    %edi
    4774:	45                   	inc    %ebp
    4775:	3a 20                	cmp    (%eax),%ah
    4777:	74 6c                	je     0x47e5
    4779:	6f                   	outsl  %ds:(%esi),(%dx)
    477a:	61                   	popa
    477b:	64 20 3c 6e          	and    %bh,%fs:(%esi,%ebp,2)
    477f:	61                   	popa
    4780:	6d                   	insl   (%dx),%es:(%edi)
    4781:	65 3e 00 4c 4f 41    	gs add %cl,%ds:0x41(%edi,%ecx,2)
    4787:	44                   	inc    %esp
    4788:	45                   	inc    %ebp
    4789:	44                   	inc    %esp
    478a:	3a 20                	cmp    (%eax),%ah
    478c:	00 45 52             	add    %al,0x52(%ebp)
    478f:	52                   	push   %edx
    4790:	3a 20                	cmp    (%eax),%ah
    4792:	46                   	inc    %esi
    4793:	49                   	dec    %ecx
    4794:	4c                   	dec    %esp
    4795:	45                   	inc    %ebp
    4796:	20 4e 4f             	and    %cl,0x4f(%esi)
    4799:	54                   	push   %esp
    479a:	20 46 4f             	and    %al,0x4f(%esi)
    479d:	55                   	push   %ebp
    479e:	4e                   	dec    %esi
    479f:	44                   	inc    %esp
    47a0:	00 57 49             	add    %dl,0x49(%edi)
    47a3:	50                   	push   %eax
    47a4:	49                   	dec    %ecx
    47a5:	4e                   	dec    %esi
    47a6:	47                   	inc    %edi
    47a7:	20 41 4c             	and    %al,0x4c(%ecx)
    47aa:	4c                   	dec    %esp
    47ab:	20 4e 45             	and    %cl,0x45(%esi)
    47ae:	55                   	push   %ebp
    47af:	52                   	push   %edx
    47b0:	41                   	inc    %ecx
    47b1:	4c                   	dec    %esp
    47b2:	20 44 41 54          	and    %al,0x54(%ecx,%eax,2)
    47b6:	41                   	inc    %ecx
    47b7:	2e 2e 2e 00 44 49 52 	cs cs add %al,%cs:0x52(%ecx,%ecx,2)
    47be:	20 2b                	and    %ch,(%ebx)
    47c0:	20 44 41 54          	and    %al,0x54(%ecx,%eax,2)
    47c4:	41                   	inc    %ecx
    47c5:	20 2b                	and    %ch,(%ebx)
    47c7:	20 52 41             	and    %dl,0x41(%edx)
    47ca:	4d                   	dec    %ebp
    47cb:	20 57 49             	and    %dl,0x49(%edi)
    47ce:	50                   	push   %eax
    47cf:	45                   	inc    %ebp
    47d0:	44                   	inc    %esp
    47d1:	2e 00 00             	add    %al,%cs:(%eax)
    47d4:	54                   	push   %esp
    47d5:	49                   	dec    %ecx
    47d6:	4d                   	dec    %ebp
    47d7:	45                   	inc    %ebp
    47d8:	52                   	push   %edx
    47d9:	20 41 43             	and    %al,0x43(%ecx)
    47dc:	54                   	push   %esp
    47dd:	49                   	dec    %ecx
    47de:	56                   	push   %esi
    47df:	45                   	inc    %ebp
    47e0:	20 2d 20 4e 45 55    	and    %ch,0x55454e20
    47e6:	52                   	push   %edx
    47e7:	4f                   	dec    %edi
    47e8:	20 43 4f             	and    %al,0x4f(%ebx)
    47eb:	52                   	push   %edx
    47ec:	45                   	inc    %ebp
    47ed:	20 50 55             	and    %dl,0x55(%eax)
    47f0:	4c                   	dec    %esp
    47f1:	53                   	push   %ebx
    47f2:	49                   	dec    %ecx
    47f3:	4e                   	dec    %esi
    47f4:	47                   	inc    %edi
    47f5:	00 00                	add    %al,(%eax)
    47f7:	00 74 73 61          	add    %dh,0x61(%ebx,%esi,2)
    47fb:	76 65                	jbe    0x4862
    47fd:	20 5b 6e             	and    %bl,0x6e(%ebx)
    4800:	61                   	popa
    4801:	6d                   	insl   (%dx),%es:(%edi)
    4802:	65 5d                	gs pop %ebp
    4804:	2c 20                	sub    $0x20,%al
    4806:	74 6c                	je     0x4874
    4808:	6f                   	outsl  %ds:(%esi),(%dx)
    4809:	61                   	popa
    480a:	64 20 5b 6e          	and    %bl,%fs:0x6e(%ebx)
    480e:	61                   	popa
    480f:	6d                   	insl   (%dx),%es:(%edi)
    4810:	65 5d                	gs pop %ebp
    4812:	2c 20                	sub    $0x20,%al
    4814:	6c                   	insb   (%dx),%es:(%edi)
    4815:	73 00                	jae    0x4817
    4817:	00 41 4c             	add    %al,0x4c(%ecx)
    481a:	4c                   	dec    %esp
    481b:	4f                   	dec    %edi
    481c:	43                   	inc    %ebx
    481d:	41                   	inc    %ecx
    481e:	54                   	push   %esp
    481f:	45                   	inc    %ebp
    4820:	44                   	inc    %esp
    4821:	20 34 4b             	and    %dh,(%ebx,%ecx,2)
    4824:	42                   	inc    %edx
    4825:	20 4e 45             	and    %cl,0x45(%esi)
    4828:	55                   	push   %ebp
    4829:	52                   	push   %edx
    482a:	41                   	inc    %ecx
    482b:	4c                   	dec    %esp
    482c:	20 50 41             	and    %dl,0x41(%eax)
    482f:	47                   	inc    %edi
    4830:	45                   	inc    %ebp
    4831:	20 41 54             	and    %al,0x54(%ecx)
    4834:	3a 20                	cmp    (%eax),%ah
    4836:	00 00                	add    %al,(%eax)
    4838:	45                   	inc    %ebp
    4839:	52                   	push   %edx
    483a:	52                   	push   %edx
    483b:	3a 20                	cmp    (%eax),%ah
    483d:	49                   	dec    %ecx
    483e:	4e                   	dec    %esi
    483f:	56                   	push   %esi
    4840:	41                   	inc    %ecx
    4841:	4c                   	dec    %esp
    4842:	49                   	dec    %ecx
    4843:	44                   	inc    %esp
    4844:	20 4f 52             	and    %cl,0x52(%edi)
    4847:	20 50 52             	and    %dl,0x52(%eax)
    484a:	4f                   	dec    %edi
    484b:	54                   	push   %esp
    484c:	45                   	inc    %ebp
    484d:	43                   	inc    %ebx
    484e:	54                   	push   %esp
    484f:	45                   	inc    %ebp
    4850:	44                   	inc    %esp
    4851:	20 41 44             	and    %al,0x44(%ecx)
    4854:	44                   	inc    %esp
    4855:	52                   	push   %edx
    4856:	00 00                	add    %al,(%eax)
    4858:	00 8e 30 00 4b 45    	add    %cl,0x454b0030(%esi)
    485e:	52                   	push   %edx
    485f:	4e                   	dec    %esi
    4860:	20 30                	and    %dh,(%eax)
    4862:	2d 31 4d 3a 00       	sub    $0x3a4d31,%eax
    4867:	23 00                	and    (%eax),%eax
    4869:	55                   	push   %ebp
    486a:	53                   	push   %ebx
    486b:	45                   	inc    %ebp
    486c:	52                   	push   %edx
    486d:	20 31                	and    %dh,(%ecx)
    486f:	4d                   	dec    %ebp
    4870:	2b 20                	sub    (%eax),%esp
    4872:	3a 00                	cmp    (%eax),%al
    4874:	2e 00 55 53          	add    %dl,%cs:0x53(%ebp)
    4878:	45                   	inc    %ebp
    4879:	44                   	inc    %esp
    487a:	3a 00                	cmp    (%eax),%al
    487c:	46                   	inc    %esi
    487d:	52                   	push   %edx
    487e:	45                   	inc    %ebp
    487f:	45                   	inc    %ebp
    4880:	3a 00                	cmp    (%eax),%al
    4882:	28 65 61             	sub    %ah,0x61(%ebp)
    4885:	63 68 20             	arpl   %bp,0x20(%eax)
    4888:	63 68 61             	arpl   %bp,0x61(%eax)
    488b:	72 3d                	jb     0x48ca
    488d:	34 4b                	xor    $0x4b,%al
    488f:	42                   	inc    %edx
    4890:	20 70 61             	and    %dh,0x61(%eax)
    4893:	67 65 29 00          	sub    %eax,%gs:(%bx,%si)
    4897:	30 31                	xor    %dh,(%ecx)
    4899:	32 33                	xor    (%ebx),%dh
    489b:	34 35                	xor    $0x35,%al
    489d:	36 37                	ss aaa
    489f:	38 39                	cmp    %bh,(%ecx)
    48a1:	41                   	inc    %ecx
    48a2:	42                   	inc    %edx
    48a3:	43                   	inc    %ebx
    48a4:	44                   	inc    %esp
    48a5:	45                   	inc    %ebp
    48a6:	46                   	inc    %esi
    48a7:	00 30                	add    %dh,(%eax)
    48a9:	00 2d 2d 2d 20 50    	add    %ch,0x50202d2d
    48af:	43                   	inc    %ebx
    48b0:	49                   	dec    %ecx
    48b1:	20 44 45 56          	and    %al,0x56(%ebp,%eax,2)
    48b5:	49                   	dec    %ecx
    48b6:	43                   	inc    %ebx
    48b7:	45                   	inc    %ebp
    48b8:	53                   	push   %ebx
    48b9:	20 2d 2d 2d 0a 00    	and    %ch,0xa2d2d
    48bf:	42                   	inc    %edx
    48c0:	3a 00                	cmp    (%eax),%al
    48c2:	20 44 3a 00          	and    %al,0x0(%edx,%edi,1)
    48c6:	20 56 45             	and    %dl,0x45(%esi)
    48c9:	4e                   	dec    %esi
    48ca:	3a 00                	cmp    (%eax),%al
    48cc:	20 44 45 56          	and    %al,0x56(%ebp,%eax,2)
    48d0:	3a 00                	cmp    (%eax),%al
    48d2:	20 43 4c             	and    %al,0x4c(%ebx)
    48d5:	53                   	push   %ebx
    48d6:	3a 00                	cmp    (%eax),%al
    48d8:	2f                   	das
    48d9:	00 20                	add    %ah,(%eax)
    48db:	3c 4e                	cmp    $0x4e,%al
    48dd:	56                   	push   %esi
    48de:	4d                   	dec    %ebp
    48df:	65 3e 00 20          	gs add %ah,%ds:(%eax)
    48e3:	3c 41                	cmp    $0x41,%al
    48e5:	48                   	dec    %eax
    48e6:	43                   	inc    %ebx
    48e7:	49                   	dec    %ecx
    48e8:	3e 00 20             	add    %ah,%ds:(%eax)
    48eb:	3c 4e                	cmp    $0x4e,%al
    48ed:	45                   	inc    %ebp
    48ee:	54                   	push   %esp
    48ef:	3e 00 20             	add    %ah,%ds:(%eax)
    48f2:	3c 56                	cmp    $0x56,%al
    48f4:	47                   	inc    %edi
    48f5:	41                   	inc    %ecx
    48f6:	3e 00 20             	add    %ah,%ds:(%eax)
    48f9:	3c 55                	cmp    $0x55,%al
    48fb:	53                   	push   %ebx
    48fc:	42                   	inc    %edx
    48fd:	3e 00 0a             	add    %cl,%ds:(%edx)
    4900:	00 4e 6f             	add    %cl,0x6f(%esi)
    4903:	20 50 43             	and    %dl,0x43(%eax)
    4906:	49                   	dec    %ecx
    4907:	20 64 65 76          	and    %ah,0x76(%ebp,%eiz,2)
    490b:	69 63 65 73 20 66 6f 	imul   $0x6f662073,0x65(%ebx),%esp
    4912:	75 6e                	jne    0x4982
    4914:	64 2e 0a 00          	fs or  %cs:(%eax),%al
	...
    4920:	58                   	pop    %eax
    4921:	02 00                	add    (%eax),%al
    4923:	00 20                	add    %ah,(%eax)
    4925:	03 00                	add    (%eax),%eax
    4927:	00 00                	add    %al,(%eax)
    4929:	00 00                	add    %al,(%eax)
    492b:	fd                   	std
    492c:	0f 00 00             	sldt   (%eax)
    492f:	00 1f                	add    %bl,(%edi)
    4931:	00 00                	add    %al,(%eax)
    4933:	00 0f                	add    %cl,(%edi)
    4935:	00 00                	add    %al,(%eax)
    4937:	00 17                	add    %dl,(%edi)
	...
    4945:	00 00                	add    %al,(%eax)
    4947:	00 ff                	add    %bh,%bh
    4949:	ff 00                	incl   (%eax)
    494b:	00 00                	add    %al,(%eax)
    494d:	9a cf 00 ff ff 00 00 	lcall  $0x0,$0xffff00cf
    4954:	00 92 cf 00 00 00    	add    %dl,0xcf(%edx)
    495a:	00 00                	add    %al,(%eax)
    495c:	00 00                	add    %al,(%eax)
    495e:	00 00                	add    %al,(%eax)
    4960:	78 00                	js     0x4962
	...
    4a86:	00 00                	add    %al,(%eax)
    4a88:	18 3c 3c             	sbb    %bh,(%esp,%edi,1)
    4a8b:	18 18                	sbb    %bl,(%eax)
    4a8d:	00 18                	add    %bl,(%eax)
    4a8f:	00 66 66             	add    %ah,0x66(%esi)
    4a92:	66 00 00             	data16 add %al,(%eax)
    4a95:	00 00                	add    %al,(%eax)
    4a97:	00 66 66             	add    %ah,0x66(%esi)
    4a9a:	ff 66 ff             	jmp    *-0x1(%esi)
    4a9d:	66 66 00 18          	data16 data16 add %bl,(%eax)
    4aa1:	7e 18                	jle    0x4abb
    4aa3:	3c 5a                	cmp    $0x5a,%al
    4aa5:	18 7e 18             	sbb    %bh,0x18(%esi)
    4aa8:	c6 c6 18             	mov    $0x18,%dh
    4aab:	30 60 c6             	xor    %ah,-0x3a(%eax)
    4aae:	c6 00 38             	movb   $0x38,(%eax)
    4ab1:	6c                   	insb   (%dx),%es:(%edi)
    4ab2:	38 76 dc             	cmp    %dh,-0x24(%esi)
    4ab5:	cc                   	int3
    4ab6:	76 00                	jbe    0x4ab8
    4ab8:	30 30                	xor    %dh,(%eax)
    4aba:	60                   	pusha
    4abb:	00 00                	add    %al,(%eax)
    4abd:	00 00                	add    %al,(%eax)
    4abf:	00 18                	add    %bl,(%eax)
    4ac1:	30 60 60             	xor    %ah,0x60(%eax)
    4ac4:	60                   	pusha
    4ac5:	30 18                	xor    %bl,(%eax)
    4ac7:	00 18                	add    %bl,(%eax)
    4ac9:	0c 06                	or     $0x6,%al
    4acb:	06                   	push   %es
    4acc:	06                   	push   %es
    4acd:	0c 18                	or     $0x18,%al
    4acf:	00 00                	add    %al,(%eax)
    4ad1:	66 3c ff             	data16 cmp $0xff,%al
    4ad4:	3c 66                	cmp    $0x66,%al
    4ad6:	00 00                	add    %al,(%eax)
    4ad8:	00 18                	add    %bl,(%eax)
    4ada:	18 7e 18             	sbb    %bh,0x18(%esi)
    4add:	18 00                	sbb    %al,(%eax)
    4adf:	00 00                	add    %al,(%eax)
    4ae1:	00 00                	add    %al,(%eax)
    4ae3:	00 00                	add    %al,(%eax)
    4ae5:	18 18                	sbb    %bl,(%eax)
    4ae7:	30 00                	xor    %al,(%eax)
    4ae9:	00 00                	add    %al,(%eax)
    4aeb:	7e 00                	jle    0x4aed
	...
    4af5:	18 18                	sbb    %bl,(%eax)
    4af7:	00 06                	add    %al,(%esi)
    4af9:	0c 18                	or     $0x18,%al
    4afb:	30 60 c0             	xor    %ah,-0x40(%eax)
    4afe:	80 00 3c             	addb   $0x3c,(%eax)
    4b01:	66 6e                	data16 outsb %ds:(%esi),(%dx)
    4b03:	7e 76                	jle    0x4b7b
    4b05:	66 3c 00             	data16 cmp $0x0,%al
    4b08:	18 38                	sbb    %bh,(%eax)
    4b0a:	18 18                	sbb    %bl,(%eax)
    4b0c:	18 18                	sbb    %bl,(%eax)
    4b0e:	7e 00                	jle    0x4b10
    4b10:	3c 66                	cmp    $0x66,%al
    4b12:	06                   	push   %es
    4b13:	0c 18                	or     $0x18,%al
    4b15:	30 7e 00             	xor    %bh,0x0(%esi)
    4b18:	7e 06                	jle    0x4b20
    4b1a:	0c 3c                	or     $0x3c,%al
    4b1c:	06                   	push   %es
    4b1d:	66 3c 00             	data16 cmp $0x0,%al
    4b20:	06                   	push   %es
    4b21:	0e                   	push   %cs
    4b22:	1e                   	push   %ds
    4b23:	36 7e 06             	ss jle 0x4b2c
    4b26:	06                   	push   %es
    4b27:	00 7e 60             	add    %bh,0x60(%esi)
    4b2a:	7c 06                	jl     0x4b32
    4b2c:	06                   	push   %es
    4b2d:	66 3c 00             	data16 cmp $0x0,%al
    4b30:	1c 30                	sbb    $0x30,%al
    4b32:	60                   	pusha
    4b33:	7c 66                	jl     0x4b9b
    4b35:	66 3c 00             	data16 cmp $0x0,%al
    4b38:	7e 06                	jle    0x4b40
    4b3a:	0c 18                	or     $0x18,%al
    4b3c:	30 30                	xor    %dh,(%eax)
    4b3e:	30 00                	xor    %al,(%eax)
    4b40:	3c 66                	cmp    $0x66,%al
    4b42:	66 3c 66             	data16 cmp $0x66,%al
    4b45:	66 3c 00             	data16 cmp $0x0,%al
    4b48:	3c 66                	cmp    $0x66,%al
    4b4a:	66 3e 06             	ds pushw %es
    4b4d:	0c 38                	or     $0x38,%al
    4b4f:	00 00                	add    %al,(%eax)
    4b51:	18 18                	sbb    %bl,(%eax)
    4b53:	00 18                	add    %bl,(%eax)
    4b55:	18 00                	sbb    %al,(%eax)
    4b57:	00 00                	add    %al,(%eax)
    4b59:	18 18                	sbb    %bl,(%eax)
    4b5b:	00 18                	add    %bl,(%eax)
    4b5d:	18 30                	sbb    %dh,(%eax)
    4b5f:	00 0c 18             	add    %cl,(%eax,%ebx,1)
    4b62:	30 60 30             	xor    %ah,0x30(%eax)
    4b65:	18 0c 00             	sbb    %cl,(%eax,%eax,1)
    4b68:	00 00                	add    %al,(%eax)
    4b6a:	7e 00                	jle    0x4b6c
    4b6c:	7e 00                	jle    0x4b6e
    4b6e:	00 00                	add    %al,(%eax)
    4b70:	30 18                	xor    %bl,(%eax)
    4b72:	0c 06                	or     $0x6,%al
    4b74:	0c 18                	or     $0x18,%al
    4b76:	30 00                	xor    %al,(%eax)
    4b78:	3c 66                	cmp    $0x66,%al
    4b7a:	06                   	push   %es
    4b7b:	0c 18                	or     $0x18,%al
    4b7d:	00 18                	add    %bl,(%eax)
	...
    4b87:	00 18                	add    %bl,(%eax)
    4b89:	3c 66                	cmp    $0x66,%al
    4b8b:	7e 66                	jle    0x4bf3
    4b8d:	66 66 00 7c 66 66    	data16 data16 add %bh,0x66(%esi,%eiz,2)
    4b93:	7c 66                	jl     0x4bfb
    4b95:	66 7c 00             	data16 jl 0x4b98
    4b98:	3c 66                	cmp    $0x66,%al
    4b9a:	60                   	pusha
    4b9b:	60                   	pusha
    4b9c:	60                   	pusha
    4b9d:	66 3c 00             	data16 cmp $0x0,%al
    4ba0:	78 6c                	js     0x4c0e
    4ba2:	66 66 66 6c          	data16 data16 data16 insb (%dx),%es:(%edi)
    4ba6:	78 00                	js     0x4ba8
    4ba8:	7e 60                	jle    0x4c0a
    4baa:	60                   	pusha
    4bab:	7c 60                	jl     0x4c0d
    4bad:	60                   	pusha
    4bae:	7e 00                	jle    0x4bb0
    4bb0:	7e 60                	jle    0x4c12
    4bb2:	60                   	pusha
    4bb3:	7c 60                	jl     0x4c15
    4bb5:	60                   	pusha
    4bb6:	60                   	pusha
    4bb7:	00 3c 66             	add    %bh,(%esi,%eiz,2)
    4bba:	60                   	pusha
    4bbb:	6e                   	outsb  %ds:(%esi),(%dx)
    4bbc:	66 66 3c 00          	data16 data16 cmp $0x0,%al
    4bc0:	66 66 66 7e 66       	data16 data16 data16 jle 0x4c2b
    4bc5:	66 66 00 3c 18       	data16 data16 add %bh,(%eax,%ebx,1)
    4bca:	18 18                	sbb    %bl,(%eax)
    4bcc:	18 18                	sbb    %bl,(%eax)
    4bce:	3c 00                	cmp    $0x0,%al
    4bd0:	1e                   	push   %ds
    4bd1:	0c 0c                	or     $0xc,%al
    4bd3:	0c 0c                	or     $0xc,%al
    4bd5:	cc                   	int3
    4bd6:	78 00                	js     0x4bd8
    4bd8:	66 6c                	data16 insb (%dx),%es:(%edi)
    4bda:	78 70                	js     0x4c4c
    4bdc:	78 6c                	js     0x4c4a
    4bde:	66 00 60 60          	data16 add %ah,0x60(%eax)
    4be2:	60                   	pusha
    4be3:	60                   	pusha
    4be4:	60                   	pusha
    4be5:	60                   	pusha
    4be6:	7e 00                	jle    0x4be8
    4be8:	63 77 7f             	arpl   %si,0x7f(%edi)
    4beb:	7f 6b                	jg     0x4c58
    4bed:	63 63 00             	arpl   %sp,0x0(%ebx)
    4bf0:	66 6e                	data16 outsb %ds:(%esi),(%dx)
    4bf2:	7e 7e                	jle    0x4c72
    4bf4:	76 66                	jbe    0x4c5c
    4bf6:	66 00 3c 66          	data16 add %bh,(%esi,%eiz,2)
    4bfa:	66 66 66 66 3c 00    	data16 data16 data16 data16 cmp $0x0,%al
    4c00:	7c 66                	jl     0x4c68
    4c02:	66 7c 60             	data16 jl 0x4c65
    4c05:	60                   	pusha
    4c06:	60                   	pusha
    4c07:	00 3c 66             	add    %bh,(%esi,%eiz,2)
    4c0a:	66 66 6e             	data16 data16 outsb %ds:(%esi),(%dx)
    4c0d:	3c 0e                	cmp    $0xe,%al
    4c0f:	00 7c 66 66          	add    %bh,0x66(%esi,%eiz,2)
    4c13:	7c 78                	jl     0x4c8d
    4c15:	6c                   	insb   (%dx),%es:(%edi)
    4c16:	66 00 3c 66          	data16 add %bh,(%esi,%eiz,2)
    4c1a:	30 18                	xor    %bl,(%eax)
    4c1c:	0c 66                	or     $0x66,%al
    4c1e:	3c 00                	cmp    $0x0,%al
    4c20:	7e 18                	jle    0x4c3a
    4c22:	18 18                	sbb    %bl,(%eax)
    4c24:	18 18                	sbb    %bl,(%eax)
    4c26:	18 00                	sbb    %al,(%eax)
    4c28:	66 66 66 66 66 66 3c 	data16 data16 data16 data16 data16 data16 cmp $0x0,%al
    4c2f:	00 
    4c30:	66 66 66 66 66 3c 18 	data16 data16 data16 data16 data16 cmp $0x18,%al
    4c37:	00 63 63             	add    %ah,0x63(%ebx)
    4c3a:	63 6b 7f             	arpl   %bp,0x7f(%ebx)
    4c3d:	77 63                	ja     0x4ca2
    4c3f:	00 66 66             	add    %ah,0x66(%esi)
    4c42:	3c 18                	cmp    $0x18,%al
    4c44:	3c 66                	cmp    $0x66,%al
    4c46:	66 00 66 66          	data16 add %ah,0x66(%esi)
    4c4a:	66 3c 18             	data16 cmp $0x18,%al
    4c4d:	18 18                	sbb    %bl,(%eax)
    4c4f:	00 7e 0c             	add    %bh,0xc(%esi)
    4c52:	18 30                	sbb    %dh,(%eax)
    4c54:	60                   	pusha
    4c55:	40                   	inc    %eax
    4c56:	7e 00                	jle    0x4c58
	...
    4c88:	00 00                	add    %al,(%eax)
    4c8a:	3c 06                	cmp    $0x6,%al
    4c8c:	3e 66 3e 00 60 60    	ds data16 add %ah,%ds:0x60(%eax)
    4c92:	7c 66                	jl     0x4cfa
    4c94:	66 66 7c 00          	data16 data16 jl 0x4c98
    4c98:	00 00                	add    %al,(%eax)
    4c9a:	3c 60                	cmp    $0x60,%al
    4c9c:	60                   	pusha
    4c9d:	66 3c 00             	data16 cmp $0x0,%al
    4ca0:	06                   	push   %es
    4ca1:	06                   	push   %es
    4ca2:	3e 66 66 66 3e 00 00 	ds data16 data16 data16 add %al,%ds:(%eax)
    4ca9:	00 3c 66             	add    %bh,(%esi,%eiz,2)
    4cac:	7e 60                	jle    0x4d0e
    4cae:	3c 00                	cmp    $0x0,%al
    4cb0:	0e                   	push   %cs
    4cb1:	18 3e                	sbb    %bh,(%esi)
    4cb3:	18 18                	sbb    %bl,(%eax)
    4cb5:	18 18                	sbb    %bl,(%eax)
    4cb7:	00 00                	add    %al,(%eax)
    4cb9:	3e 66 66 3e 06       	ds data16 ds pushw %es
    4cbe:	3c 00                	cmp    $0x0,%al
    4cc0:	60                   	pusha
    4cc1:	60                   	pusha
    4cc2:	7c 66                	jl     0x4d2a
    4cc4:	66 66 66 00 18       	data16 data16 data16 add %bl,(%eax)
    4cc9:	00 38                	add    %bh,(%eax)
    4ccb:	18 18                	sbb    %bl,(%eax)
    4ccd:	18 3c 00             	sbb    %bh,(%eax,%eax,1)
    4cd0:	06                   	push   %es
    4cd1:	00 06                	add    %al,(%esi)
    4cd3:	06                   	push   %es
    4cd4:	06                   	push   %es
    4cd5:	66 3c 00             	data16 cmp $0x0,%al
    4cd8:	60                   	pusha
    4cd9:	60                   	pusha
    4cda:	66 6c                	data16 insb (%dx),%es:(%edi)
    4cdc:	78 6c                	js     0x4d4a
    4cde:	66 00 38             	data16 add %bh,(%eax)
    4ce1:	18 18                	sbb    %bl,(%eax)
    4ce3:	18 18                	sbb    %bl,(%eax)
    4ce5:	18 3c 00             	sbb    %bh,(%eax,%eax,1)
    4ce8:	00 00                	add    %al,(%eax)
    4cea:	66 7f 7f             	data16 jg 0x4d6c
    4ced:	6b 63 00 00          	imul   $0x0,0x0(%ebx),%esp
    4cf1:	00 7c 66 66          	add    %bh,0x66(%esi,%eiz,2)
    4cf5:	66 66 00 00          	data16 data16 add %al,(%eax)
    4cf9:	00 3c 66             	add    %bh,(%esi,%eiz,2)
    4cfc:	66 66 3c 00          	data16 data16 cmp $0x0,%al
    4d00:	00 00                	add    %al,(%eax)
    4d02:	7c 66                	jl     0x4d6a
    4d04:	66 7c 60             	data16 jl 0x4d67
    4d07:	60                   	pusha
    4d08:	00 00                	add    %al,(%eax)
    4d0a:	3e 66 66 3e 06       	ds data16 ds pushw %es
    4d0f:	06                   	push   %es
    4d10:	00 00                	add    %al,(%eax)
    4d12:	7c 66                	jl     0x4d7a
    4d14:	60                   	pusha
    4d15:	60                   	pusha
    4d16:	60                   	pusha
    4d17:	00 00                	add    %al,(%eax)
    4d19:	00 3e                	add    %bh,(%esi)
    4d1b:	60                   	pusha
    4d1c:	3c 06                	cmp    $0x6,%al
    4d1e:	7c 00                	jl     0x4d20
    4d20:	18 18                	sbb    %bl,(%eax)
    4d22:	7e 18                	jle    0x4d3c
    4d24:	18 18                	sbb    %bl,(%eax)
    4d26:	0e                   	push   %cs
    4d27:	00 00                	add    %al,(%eax)
    4d29:	00 66 66             	add    %ah,0x66(%esi)
    4d2c:	66 66 3e 00 00       	data16 data16 add %al,%ds:(%eax)
    4d31:	00 66 66             	add    %ah,0x66(%esi)
    4d34:	66 3c 18             	data16 cmp $0x18,%al
    4d37:	00 00                	add    %al,(%eax)
    4d39:	00 63 6b             	add    %ah,0x6b(%ebx)
    4d3c:	7f 3e                	jg     0x4d7c
    4d3e:	36 00 00             	add    %al,%ss:(%eax)
    4d41:	00 66 3c             	add    %ah,0x3c(%esi)
    4d44:	18 3c 66             	sbb    %bh,(%esi,%eiz,2)
    4d47:	00 00                	add    %al,(%eax)
    4d49:	00 66 66             	add    %ah,0x66(%esi)
    4d4c:	66 3e 06             	ds pushw %es
    4d4f:	3c 00                	cmp    $0x0,%al
    4d51:	00 7e 0c             	add    %bh,0xc(%esi)
    4d54:	18 30                	sbb    %dh,(%eax)
    4d56:	7e 00                	jle    0x4d58
	...
