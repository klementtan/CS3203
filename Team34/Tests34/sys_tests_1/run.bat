@echo off
title system test 1 queries

..\..\Code34\build\src\autotester\Release\autotester.exe Source.simple Follows.txt output-Follows.xml
..\..\Code34\build\src\autotester\Release\autotester.exe Source.simple FollowsT.txt output-FollowsT.xml
..\..\Code34\build\src\autotester\Release\autotester.exe Source.simple Parent.txt output-Parent.xml
..\..\Code34\build\src\autotester\Release\autotester.exe Source.simple ParentT.txt output-ParentT.xml
..\..\Code34\build\src\autotester\Release\autotester.exe Source.simple Uses.txt output-Uses.xml
..\..\Code34\build\src\autotester\Release\autotester.exe Source.simple Modifies.txt output-Modifies.xml
..\..\Code34\build\src\autotester\Release\autotester.exe Source.simple NoClause.txt output-NoClause.xml
..\..\Code34\build\src\autotester\Release\autotester.exe Source.simple Pattern.txt output-Pattern.xml

echo FINISHED
exit
