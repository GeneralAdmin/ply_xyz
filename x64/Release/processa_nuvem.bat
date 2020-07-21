@echo off
ECHO Aguarde conversão de arquivos.
ConsoleApplication2.exe rec1_.ply rec2_.ply
del nvm1_tmp.ply
del nvm2_tmp.ply
del nvm3_tmp.ply
ECHO Arquivos convertidos.
m3c2 0.4 : 0.04 : 0.8 : nvm2.xyz nvm1.xyz nvm2.xyz nvm3.xyz result.xyz
del nvm1.xyz
del nvm2.xyz
del nvm3.xyz
ECHO Resultados prontos!
cls
call rake
pause