
@echo off
title system test 1 queries

..\..\Code34\build_win\x86-Debug\src\autotester\autotester.exe Source.txt Follows.txt outputFollows.xml
..\..\Code34\build_win\x86-Debug\src\autotester\autotester.exe Source.txt FollowsT.txt outputFollowsT.xml
..\..\Code34\build_win\x86-Debug\src\autotester\autotester.exe Source.txt Parent.txt outputParent.xml
..\..\Code34\build_win\x86-Debug\src\autotester\autotester.exe Source.txt ParentT.txt outputParentT.xml
..\..\Code34\build_win\x86-Debug\src\autotester\autotester.exe Source.txt Uses.txt outputUses.xml
..\..\Code34\build_win\x86-Debug\src\autotester\autotester.exe Source.txt Modifies.txt outputModifies.xml
..\..\Code34\build_win\x86-Debug\src\autotester\autotester.exe Source.txt NoClause.txt outputNoClause.xml
..\..\Code34\build_win\x86-Debug\src\autotester\autotester.exe Source.txt Pattern.txt outputPattern.xml

echo FINISHED
pause
exit
