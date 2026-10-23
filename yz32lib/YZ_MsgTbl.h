#ifndef  __YZ_MagTbl_h__
#define  __YZ_MagTbl_h__

#ifdef WIN32

  // WIN32 
  #define YZ_TMP  "C:\\TMP\\"

#else  

  //  UNIX  
  #define YZ_TMP  "/tmp/t/"
  
#endif


#ifndef YZ1DLLEXP
	#ifdef YZ1_STATIC
		#define YZ1DLLEXP
	#elif defined(YZ1_EXPORTS)
		#define YZ1DLLEXP __declspec(dllexport) 
	#else
		#define YZ1DLLEXP __declspec(dllimport) 
	#endif
#endif

#ifndef YZ_EXT1
  #define YZ_EXT1 ".yz1"
#endif

//---------------------------------------------------------

class YZ1DLLEXP YzMessageTable
{
    public: static const char *  M01() ; // "解凍ファイルの出力フォルダを選んでください。"
    public: static const char *  M02() ; // "       : A> yzdec FileName.yz1           (ファイル'FileName.yz1'を解凍)\n"
    public: static const char *  M10() ; // "圧縮ファイルの出力フォルダを選んでください。"
    public: static const char *  M11() ; // "       : A> yzenc FileName.txt           (ファイル'FileName.txt'を圧縮)\n"
    public: static const char *  M19() ; // "解凍テストを終了しました。\n問題ありません。(処理時間：%s 秒)"
    public: static const char *  M20() ; // "ファイル(%s)が入力(fopen)できません。\n"  "処理を中止します。"
    public: static const char *  M21() ; // "ファイル(%s)が入力(fread)できません。\n"  "処理を中止します。"
    public: static const char *  M22() ; // "ファイル(%s)はＹＺ圧縮されているファイルではありません。\n" "解凍処理を中止します。"
    public: static const char *  M23() ; // "パスワードを入力して下さい。"
    public: static const char *  M24() ; // "パスワードが入力されませんでした。\n処理を中止します。"
    public: static const char *  M25() ; // "パスワードが一致しません。\n処理を中止します。"
    public: static const char *  M26() ; // "解凍先のメモリ(%s byte)が確保できません。\n"  "処理を中止します。"
    public: static const char *  M27() ; // "ファイル(%s file)が既に存在します。\n" "上書きしますか？"
    public: static const char *  M28() ; // "ファイルの内容と圧縮ファイルの内容を比較しますか？"
    public: static const char *  M29() ; // "ファイルを出力せずに終了します。"
    public: static const char *  M31() ; // "入力データが異常である。\n"  "入力データ内にエリアを超えて解凍しようとするコードがある。\n" "ファイルを比較せずに終了します。"
    public: static const char *  M32() ; // "入力データが異常である。\n" "入力データ内に存在しないデータを示すコードがある。\n" "ファイルを比較せずに終了します。"
    public: static const char *  M33() ; // "比較処理を終了しました。\nファイルの内容と一致します。(処理時間：%s 秒)"
    public: static const char *  M34() ; // "比較処理を終了しました。\nファイルの内容と一致しませんでした。(処理時間：%s 秒)"
    public: static const char *  M38() ; // "伸長処理を終了しました。(処理時間：%s 秒)"
    public: static const char *  M39() ; // "拡張子が '" YZ_EXT1 "' のファイル(%s%s)は処理できません。\n" "拡張子を変更して下さい。"
    public: static const char *  M40() ; // "入力ファイル(%s%s)と同じファイルに出力できません。\n" "ファイルの出力先を変更して下さい。"
    public: static const char *  M41() ; // "出力ファイル(%s)が既に存在します。\n" "上書きしますか？"
    public: static const char *  M42() ; // "ファイル(%s)を出力せずに終了します。"
    public: static const char *  M43() ; // "ファイル(%s)が出力(fopen)できません。\n" "処理を中止します。"
    public: static const char *  M46() ; // "確認のため、もう一度入力して下さい。"
    public: static const char *  M48() ; // "パスワードが一致しません。\n" "もう一度、入力し直してください。"
    public: static const char *  M49() ; // "パスワードが入力処理がキャンセルされました。\n処理を中止します。"
    public: static const char *  M50() ; // "伸長時のパスワードは、\n\n%s\n\nになります。\n" "記録して下さい。"
    public: static const char *  M51() ; // "圧縮元のメモリ(%s byte)が確保できません。\n"  "処理を中止します。"
    public: static const char *  M54() ; // "入力データの偏りが大きくデータテーブルが作成できません。\n" "入力データを変更するか複数に分けてください。\n" "処理を中止します。"
    public: static const char *  M55() ; // "ファイル(%s)が出力(fwrite)できません。\n" "処理を中止します。"
    public: static const char *  M56() ; // "out name: %s (圧縮不可:%s, %s byte)"
    public: static const char *  M57() ; // "入力データが少ないかデータが複雑なため圧縮できませんでした。\n" "圧縮ファイル(%s)の\n" "サイズは入力データより大きくなります。ファイルを出力しますか？。"
    public: static const char *  M59() ; // "圧縮処理を終了しました。(処理時間：%s 秒)"
    public: static const char *  M70() ; // "比較ファイルの内容と圧縮ファイルの内容が不一致です。\n" "比較ファイル(%s)は存在しません。\n" "比較処理を続けますか？"
    public: static const char *  M71() ; // "比較ファイルの内容と圧縮ファイルの内容が不一致です。\n" "比較ファイル(%s)のサイズと圧縮ファイル内のファイルサイズ\n" "(%s byte, %s byte)が異なります。\n" "比較処理を続けますか？"
    public: static const char *  M72() ; // "比較ファイルの内容と圧縮ファイルの内容が不一致です。\n" "比較ファイル(%s)の更新日付が異なります。\n" "比較ファイル：%s" "圧縮ファイル：%s" "比較処理を続けますか？"
    public: static const char *  M73() ; // "比較用のメモリ(%s byte)が確保できません。\n" "処理を中止します。"
    public: static const char *  M76() ; // "比較ファイルの内容と圧縮ファイルの内容が不一致です。\n" "比較ファイル(%s)の %s byte 目のデータが一致しない。\n" "%s\n" "比較処理を続けますか？"
    public: static const char *  M77() ; // "ディレクトリ(%s)が作成(mkdir)できません。\n" "処理を中止します。"
    public: static const char *  M80() ; // "ファイル(%s)の更新日付が設定(utime)できません。\n" "処理は続行します。"
    public: static const char *  M90() ; // "ヘッダ用のメモリ(%s byte)が確保できません。\n" "処理を中止します。"
    public: static const char *  M91() ; // "ファイル(%s)が入力(stat)できません。\n" "処理を中止します。"
    // kinaba
    public: static const char *  M92() ; // "ヘッダが壊れています。\n" "処理を中止します。"
} ;

#define YZ_MSG_TBL_01 (YzMessageTable::M01()) // "解凍ファイルの出力フォルダを選んでください。"
#define YZ_MSG_TBL_02 (YzMessageTable::M02()) // "       : A> yzdec FileName.yz1           (ファイル'FileName.yz1'を解凍)\n"
#define YZ_MSG_TBL_10 (YzMessageTable::M10()) // "圧縮ファイルの出力フォルダを選んでください。"
#define YZ_MSG_TBL_11 (YzMessageTable::M11()) // "       : A> yzenc FileName.txt           (ファイル'FileName.txt'を圧縮)\n"
#define YZ_MSG_TBL_19 (YzMessageTable::M19()) // "解凍テストを終了しました。\n問題ありません。(処理時間：%s 秒)"
#define YZ_MSG_TBL_20 (YzMessageTable::M20()) // "ファイル(%s)が入力(fopen)できません。\n"  "処理を中止します。"
#define YZ_MSG_TBL_21 (YzMessageTable::M21()) // "ファイル(%s)が入力(fread)できません。\n"  "処理を中止します。"
#define YZ_MSG_TBL_22 (YzMessageTable::M22()) // "ファイル(%s)はＹＺ圧縮されているファイルではありません。\n" "解凍処理を中止します。"
#define YZ_MSG_TBL_23 (YzMessageTable::M23()) // "パスワードを入力して下さい。"
#define YZ_MSG_TBL_24 (YzMessageTable::M24()) // "パスワードが入力されませんでした。\n処理を中止します。"
#define YZ_MSG_TBL_25 (YzMessageTable::M25()) // "パスワードが一致しません。\n処理を中止します。"
#define YZ_MSG_TBL_26 (YzMessageTable::M26()) // "解凍先のメモリ(%s byte)が確保できません。\n"  "処理を中止します。"
#define YZ_MSG_TBL_27 (YzMessageTable::M27()) // "ファイル(%s file)が既に存在します。\n" "上書きしますか？"
#define YZ_MSG_TBL_28 (YzMessageTable::M28()) // "ファイルの内容と圧縮ファイルの内容を比較しますか？"
#define YZ_MSG_TBL_29 (YzMessageTable::M29()) // "ファイルを出力せずに終了します。"
#define YZ_MSG_TBL_31 (YzMessageTable::M31()) // "入力データが異常である。\n"  "入力データ内にエリアを超えて解凍しようとするコードがある。\n" "ファイルを比較せずに終了します。"
#define YZ_MSG_TBL_32 (YzMessageTable::M32()) // "入力データが異常である。\n" "入力データ内に存在しないデータを示すコードがある。\n" "ファイルを比較せずに終了します。"
#define YZ_MSG_TBL_33 (YzMessageTable::M33()) // "比較処理を終了しました。\nファイルの内容と一致します。(処理時間：%s 秒)"
#define YZ_MSG_TBL_34 (YzMessageTable::M34()) // "比較処理を終了しました。\nファイルの内容と一致しませんでした。(処理時間：%s 秒)"
#define YZ_MSG_TBL_38 (YzMessageTable::M38()) // "伸長処理を終了しました。(処理時間：%s 秒)"
#define YZ_MSG_TBL_39 (YzMessageTable::M39()) // "拡張子が '" YZ_EXT1 "' のファイル(%s%s)は処理できません。\n" "拡張子を変更して下さい。"
#define YZ_MSG_TBL_40 (YzMessageTable::M40()) // "入力ファイル(%s%s)と同じファイルに出力できません。\n" "ファイルの出力先を変更して下さい。"
#define YZ_MSG_TBL_41 (YzMessageTable::M41()) // "出力ファイル(%s)が既に存在します。\n" "上書きしますか？"
#define YZ_MSG_TBL_42 (YzMessageTable::M42()) // "ファイル(%s)を出力せずに終了します。"
#define YZ_MSG_TBL_43 (YzMessageTable::M43()) // "ファイル(%s)が出力(fopen)できません。\n" "処理を中止します。"
#define YZ_MSG_TBL_46 (YzMessageTable::M46()) // "確認のため、もう一度入力して下さい。"
#define YZ_MSG_TBL_48 (YzMessageTable::M48()) // "パスワードが一致しません。\n" "もう一度、入力し直してください。"
#define YZ_MSG_TBL_49 (YzMessageTable::M49()) // "パスワードが入力処理がキャンセルされました。\n処理を中止します。"
#define YZ_MSG_TBL_50 (YzMessageTable::M50()) // "伸長時のパスワードは、\n\n%s\n\nになります。\n" "記録して下さい。"
#define YZ_MSG_TBL_51 (YzMessageTable::M51()) // "圧縮元のメモリ(%s byte)が確保できません。\n"  "処理を中止します。"
#define YZ_MSG_TBL_54 (YzMessageTable::M54()) // "入力データの偏りが大きくデータテーブルが作成できません。\n" "入力データを変更するか複数に分けてください。\n" "処理を中止します。"
#define YZ_MSG_TBL_55 (YzMessageTable::M55()) // "ファイル(%s)が出力(fwrite)できません。\n" "処理を中止します。"
#define YZ_MSG_TBL_56 (YzMessageTable::M56()) // "out name: %s (圧縮不可:%s, %s byte)"
#define YZ_MSG_TBL_57 (YzMessageTable::M57()) // "入力データが少ないかデータが複雑なため圧縮できませんでした。\n" "圧縮ファイル(%s)の\n" "サイズは入力データより大きくなります。ファイルを出力しますか？。"
#define YZ_MSG_TBL_59 (YzMessageTable::M59()) // "圧縮処理を終了しました。(処理時間：%s 秒)"
#define YZ_MSG_TBL_70 (YzMessageTable::M70()) // "比較ファイルの内容と圧縮ファイルの内容が不一致です。\n" "比較ファイル(%s)は存在しません。\n" "比較処理を続けますか？"
#define YZ_MSG_TBL_71 (YzMessageTable::M71()) // "比較ファイルの内容と圧縮ファイルの内容が不一致です。\n" "比較ファイル(%s)のサイズと圧縮ファイル内のファイルサイズ\n" "(%s byte, %s byte)が異なります。\n" "比較処理を続けますか？"
#define YZ_MSG_TBL_72 (YzMessageTable::M72()) // "比較ファイルの内容と圧縮ファイルの内容が不一致です。\n" "比較ファイル(%s)の更新日付が異なります。\n" "比較ファイル：%s" "圧縮ファイル：%s" "比較処理を続けますか？"
#define YZ_MSG_TBL_73 (YzMessageTable::M73()) // "比較用のメモリ(%s byte)が確保できません。\n" "処理を中止します。"
#define YZ_MSG_TBL_76 (YzMessageTable::M76()) // "比較ファイルの内容と圧縮ファイルの内容が不一致です。\n" "比較ファイル(%s)の %s byte 目のデータが一致しない。\n" "%s\n" "比較処理を続けますか？"
#define YZ_MSG_TBL_77 (YzMessageTable::M77()) // "ディレクトリ(%s)が作成(mkdir)できません。\n" "処理を中止します。"
#define YZ_MSG_TBL_80 (YzMessageTable::M80()) // "ファイル(%s)の更新日付が設定(utime)できません。\n" "処理は続行します。"
#define YZ_MSG_TBL_90 (YzMessageTable::M90()) // "ヘッダ用のメモリ(%s byte)が確保できません。\n" "処理を中止します。"
#define YZ_MSG_TBL_91 (YzMessageTable::M91()) // "ファイル(%s)が入力(stat)できません。\n" "処理を中止します。"
// kinaba
#define YZ_MSG_TBL_92 (YzMessageTable::M92()) // "ヘッダが壊れています。\n" "処理を中止します。"


#define YZ_MSG_NO_01 (   1) // "解凍ファイルの出力フォルダを選んでください。"
#define YZ_MSG_NO_02 (   2) // "       : A> yzdec file.yz1           (ファイル'file.yz1'を解凍)\n"
#define YZ_MSG_NO_10 (  10) // "圧縮ファイルの出力フォルダを選んでください。"
#define YZ_MSG_NO_11 (  11) // "       : A> yzenc file.txt           (ファイル'file.txt'を圧縮)\n"
#define YZ_MSG_NO_19 (1019) // MessINF "解凍テストを終了しました。\n問題ありません。(処理時間：%s 秒)"
#define YZ_MSG_NO_20 (2020) // MessEX "ファイル(%s)が入力(fopen)できません。\n"  "処理を中止します。"
#define YZ_MSG_NO_21 (2021) // MessEX "ファイル(%s)が入力(fread)できません。\n"  "処理を中止します。"
#define YZ_MSG_NO_22 (2022) // MessEX "ファイル(%s)はＹＺ圧縮されているファイルではありません。\n" "解凍処理を中止します。"
#define YZ_MSG_NO_23 (3023) // MessINPUT "パスワードを入力して下さい。"
#define YZ_MSG_NO_24 (1024) // MessINF "パスワードが入力されませんでした。\n処理を中止します。"
#define YZ_MSG_NO_25 (1025) // MessINF "パスワードが一致しません。\n処理を中止します。"
#define YZ_MSG_NO_26 (2026) // MessEX "解凍先のメモリ(%s byte)が確保できません。\n"  "処理を中止します。"
#define YZ_MSG_NO_27 (4027) // MessYN "ファイル(%s file)が既に存在します。\n" "上書きしますか？"
#define YZ_MSG_NO_28 (4028) // MessYN "ファイルの内容と圧縮ファイルの内容を比較しますか？"
#define YZ_MSG_NO_29 (1029) // MessINF "ファイルを出力せずに終了します。"
#define YZ_MSG_NO_31 (2031) // MessEX "入力データが異常である。\n"  "入力データ内にエリアを超えて解凍しようとするコードがある。\n" "ファイルを比較せずに終了します。"
#define YZ_MSG_NO_32 (2032) // MessEX "入力データが異常である。\n" "入力データ内に存在しないデータを示すコードがある。\n" "ファイルを比較せずに終了します。"
#define YZ_MSG_NO_33 (1033) // MessINF "比較処理を終了しました。\nファイルの内容と一致します。(処理時間：%s 秒)"
#define YZ_MSG_NO_34 (1034) // MessINF "比較処理を終了しました。\nファイルの内容と一致しませんでした。(処理時間：%s 秒)"
#define YZ_MSG_NO_38 (1038) // MessINF "伸長処理を終了しました。(処理時間：%s 秒)"
#define YZ_MSG_NO_39 (2039) // MessEX "拡張子が '" YZ_EXT1 "' のファイル(%s%s)は処理できません。\n" "拡張子を変更して下さい。"
#define YZ_MSG_NO_40 (2040) // MessEX "入力ファイル(%s%s)と同じファイルに出力できません。\n" "ファイルの出力先を変更して下さい。"
#define YZ_MSG_NO_41 (4041) // MessYN "出力ファイル(%s)が既に存在します。\n" "上書きしますか？"
#define YZ_MSG_NO_42 (1042) // MessINF "ファイル(%s)を出力せずに終了します。"
#define YZ_MSG_NO_43 (2043) // MessEX "ファイル(%s)が出力(fopen)できません。\n" "処理を中止します。"
#define YZ_MSG_NO_46 (3046) // MessINPUT "確認のため、もう一度入力して下さい。"
#define YZ_MSG_NO_48 (4048) // MessYN "パスワードが一致しません。\n" "もう一度、入力し直してください。"
#define YZ_MSG_NO_49 (1049) // MessINF "パスワードが入力処理がキャンセルされました。\n処理を中止します。"
#define YZ_MSG_NO_50 (1050) // MessINF "伸長時のパスワードは、\n\n%s\n\nになります。\n" "記録して下さい。"
#define YZ_MSG_NO_51 (2051) // MessEX "圧縮元のメモリ(%s byte)が確保できません。\n"  "処理を中止します。"
#define YZ_MSG_NO_54 (2054) // MessEX "入力データの偏りが大きくデータテーブルが作成できません。\n" "入力データを変更するか複数に分けてください。\n" "処理を中止します。"
#define YZ_MSG_NO_55 (2055) // MessEX "ファイル(%s)が出力(fwrite)できません。\n" "処理を中止します。"
#define YZ_MSG_NO_56 (  56) // "out name: %s (圧縮不可:%s, %s byte)"
#define YZ_MSG_NO_57 (4057) // MessYN "入力データが少ないかデータが複雑なため圧縮できませんでした。\n" "圧縮ファイル(%s)の\n" "サイズは入力データより大きくなります。ファイルを出力しますか？。"
#define YZ_MSG_NO_59 (1059) // MessINF "圧縮処理を終了しました。(処理時間：%s 秒)"
#define YZ_MSG_NO_70 (4070) // MessYN "比較ファイルの内容と圧縮ファイルの内容が不一致です。\n" "比較ファイル(%s)は存在しません。\n" "比較処理を続けますか？"
#define YZ_MSG_NO_71 (4071) // MessYN "比較ファイルの内容と圧縮ファイルの内容が不一致です。\n" "比較ファイル(%s)のサイズと圧縮ファイル内のファイルサイズ\n" "(%s byte, %s byte)が異なります。\n" "比較処理を続けますか？"
#define YZ_MSG_NO_72 (4072) // MessYN "比較ファイルの内容と圧縮ファイルの内容が不一致です。\n" "比較ファイル(%s)の更新日付が異なります。\n" "比較ファイル：%s" "圧縮ファイル：%s" "比較処理を続けますか？"
#define YZ_MSG_NO_73 (2073) // MessEX "比較用のメモリ(%s byte)が確保できません。\n" "処理を中止します。"
#define YZ_MSG_NO_76 (4076) // MessYN "比較ファイルの内容と圧縮ファイルの内容が不一致です。\n" "比較ファイル(%s)の %s byte 目のデータが一致しない。\n" "%s\n" "比較処理を続けますか？"
#define YZ_MSG_NO_77 (2077) // MessEX "ディレクトリ(%s)が作成(mkdir)できません。\n" "処理を中止します。"
#define YZ_MSG_NO_80 (1080) // MessINF "ファイル(%s)の更新日付が設定(utime)できません。\n" "処理は続行します。"
#define YZ_MSG_NO_90 (2090) // MessEX "ヘッダ用のメモリ(%s byte)が確保できません。\n" "処理を中止します。"
#define YZ_MSG_NO_91 (2091) // MessEX "ファイル(%s)が入力(stat)できません。\n" "処理を中止します。"
// kinaba
#define YZ_MSG_NO_92 (2092) // MessEX "ヘッダが壊れています。\n" "処理を中止します。"


#endif // __YZ_MagTbl_h__
