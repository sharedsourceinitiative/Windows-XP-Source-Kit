;       Command.com initialization messages.
;
;       See resmsg.equ for macro definition.

	msg     BadVerMsg,   <"������⤞ ⡛��� ��� DOS",CR,LF>
	msg     OutEnvMsg,   <"� ��㣞 �����ᢢ����� ������㟞��",CR,LF>
        msg     CopyrightMsg,<"Microsoft(R) Windows DOS",CR,LF,"(C)���������� �����飘�� Microsoft Corp 1990-2001.",CR,LF>
	msg     BadComLkMsg, <"� �������⤦� ���ᢦ��� ����㫞��� ��� COMMAND �夘� ����ᢢ����",CR,LF>
	msg     BadComAccMsg,<"� �������⤦� ���ᢦ��� ����㫞��� ��� COMMAND �夘� ����ᢢ����",CR,LF,"��� �����᧞�� � ��橙���",CR,LF>

	msg     Help1Msg  <"���夞�� �⦬ interpreter ������ ��� MS-DOS.",CR,LF,CR,LF>
	msg     Help2Msg  <"COMMAND [[�婡��:]��������] [�������] [/E:nnnnn] [/P] [/C ��������] [/MSG]",CR,LF,CR,LF>
	msg     Help3Msg  <"  [�婡��:]��������    � ���ᢦ��� ���� ���� ��婡���� �� ����� COMMAND.COM.",CR,LF>
	msg     Help4Msg  <"  �������         ������� ��� �婦��/⥦�� ��� ������.",CR,LF>
	msg     Help5Msg  <"  /E:nnnnn        ������ �⚜��� �����ᢢ����� �� nnnnn byte.",CR,LF>
	msg     Help6Msg  <"  /P              ������ ��� �⦬ interpreter � �椠���",CR,LF,"(��� �����櫞�� ��曦�).",CR,LF>
	msg     Help7Msg  <"  /C ��������    ���⢜�� ��� ������ ��� �����婫��� ��� ��� ��������,",CR,LF,"��� ���� ��穞.",CR,LF>
	msg     Help8Msg  <"  /MSG            �����坜� �� 梘 �� ���磘�� ��ᢣ���� �夘� ���������⤘",CR,LF>
	msg     Help9Msg  <"                  ��� ��㣞. ��⧜� �� �����婜�� ��� ����槫� /P ��高.",CR,LF>

HelpMsgs        dw      RESGROUP:Help1Msg,RESGROUP:Help2Msg
		dw      RESGROUP:Help3Msg,RESGROUP:Help4Msg
		dw      RESGROUP:Help5Msg,RESGROUP:Help6Msg
		dw      RESGROUP:Help7Msg,RESGROUP:Help8Msg
		dw      RESGROUP:Help9Msg,0

	public  HelpMsgs
