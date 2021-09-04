# WiMuseTN (MSX-WiMuse TangNano unit)

## Gowin
Gowin_V1.9.7.06Beta

## TangNanoプロジェクトファイルの作成
Series: GW1N
GW1N-LV1QN48C6/I5

## プロジェクト設定・デフォルトからの変更点
Menu→Project→Configuration
	Synthesize
		General
			Verilog Language: System Verilog 2017
	Place&Route
		Dual-Purpose Pin
			［　］Use JTag as regular IO	← 絶対にレ点を入れない。USBから書き込めなくなる
			［レ］Use SSPI 〃
			［レ］Use MSPI 〃
			［レ］Use READY 〃
			［レ］Use DONE 〃
			［レ］Use RECONFIG_N 〃
			［レ］Use MODE 〃
			［　］Use I2C 〃

## コンパイルと実機への書き込み方法
1. Process→Syhnthesize を右クリックして "RUN"
2. Place&Route を右クリックして "RUN"
3. Program Device をダブルクリック→GOWIN Programmer が起動する
4. GOWIN Programmerで、GW1N を右クリックして、Cable Setting → Cableを選び、
	Cable Settingウィンドウが開くので、Frequency を、2MHzから、15Mhzに変更する。
5. Program/Configure を実行する。
	問題なければ実機への書き込みが開始される。


以上
