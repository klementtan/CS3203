
@echo off
title system test 1 queries

..\..\Code34\build_win\x86-Debug\src\autotester\autotester.exe Source.simple Follows.txt outputFollows.xml
..\..\Code34\build_win\x86-Debug\src\autotester\autotester.exe Source.simple FollowsT.txt outputFollowsT.xml
..\..\Code34\build_win\x86-Debug\src\autotester\autotester.exe Source.simple Parent.txt outputParent.xml
..\..\Code34\build_win\x86-Debug\src\autotester\autotester.exe Source.simple ParentT.txt outputParentT.xml
..\..\Code34\build_win\x86-Debug\src\autotester\autotester.exe Source.simple Uses.txt outputUses.xml
..\..\Code34\build_win\x86-Debug\src\autotester\autotester.exe Source.simple Modifies.txt outputModifies.xml
..\..\Code34\build_win\x86-Debug\src\autotester\autotester.exe Source.simple NoClause.txt outputNoClause.xml
..\..\Code34\build_win\x86-Debug\src\autotester\autotester.exe Source.simple Pattern.txt outputPattern.xml

echo FINISHED
pause
exit
