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
    public: static const char *  M01() ; // "�𓀃t�@�C���̏o�̓t�H���_��I��ł��������B"
    public: static const char *  M02() ; // "       : A> yzdec FileName.yz1           (�t�@�C��'FileName.yz1'����)\n"
    public: static const char *  M10() ; // "���k�t�@�C���̏o�̓t�H���_��I��ł��������B"
    public: static const char *  M11() ; // "       : A> yzenc FileName.txt           (�t�@�C��'FileName.txt'�����k)\n"
    public: static const char *  M19() ; // "�𓀃e�X�g���I�����܂����B\n��肠��܂���B(�������ԁF%s �b)"
    public: static const char *  M20() ; // "�t�@�C��(%s)������(fopen)�ł��܂���B\n"  "�����𒆎~���܂��B"
    public: static const char *  M21() ; // "�t�@�C��(%s)������(fread)�ł��܂���B\n"  "�����𒆎~���܂��B"
    public: static const char *  M22() ; // "�t�@�C��(%s)�͂x�y���k����Ă���t�@�C���ł͂���܂���B\n" "�𓀏����𒆎~���܂��B"
    public: static const char *  M23() ; // "�p�X���[�h����͂��ĉ������B"
    public: static const char *  M24() ; // "�p�X���[�h�����͂���܂���ł����B\n�����𒆎~���܂��B"
    public: static const char *  M25() ; // "�p�X���[�h����v���܂���B\n�����𒆎~���܂��B"
    public: static const char *  M26() ; // "�𓀐�̃�����(%s byte)���m�ۂł��܂���B\n"  "�����𒆎~���܂��B"
    public: static const char *  M27() ; // "�t�@�C��(%s file)�����ɑ��݂��܂��B\n" "�㏑�����܂����H"
    public: static const char *  M28() ; // "�t�@�C���̓��e�ƈ��k�t�@�C���̓��e���r���܂����H"
    public: static const char *  M29() ; // "�t�@�C�����o�͂����ɏI�����܂��B"
    public: static const char *  M31() ; // "���̓f�[�^���ُ�ł���B\n"  "���̓f�[�^���ɃG���A�𒴂��ĉ𓀂��悤�Ƃ���R�[�h������B\n" "�t�@�C�����r�����ɏI�����܂��B"
    public: static const char *  M32() ; // "���̓f�[�^���ُ�ł���B\n" "���̓f�[�^���ɑ��݂��Ȃ��f�[�^�������R�[�h������B\n" "�t�@�C�����r�����ɏI�����܂��B"
    public: static const char *  M33() ; // "��r�������I�����܂����B\n�t�@�C���̓��e�ƈ�v���܂��B(�������ԁF%s �b)"
    public: static const char *  M34() ; // "��r�������I�����܂����B\n�t�@�C���̓��e�ƈ�v���܂���ł����B(�������ԁF%s �b)"
    public: static const char *  M38() ; // "�L���������I�����܂����B(�������ԁF%s �b)"
    public: static const char *  M39() ; // "�g���q�� '" YZ_EXT1 "' �̃t�@�C��(%s%s)�͏����ł��܂���B\n" "�g���q��ύX���ĉ������B"
    public: static const char *  M40() ; // "���̓t�@�C��(%s%s)�Ɠ����t�@�C���ɏo�͂ł��܂���B\n" "�t�@�C���̏o�͐��ύX���ĉ������B"
    public: static const char *  M41() ; // "�o�̓t�@�C��(%s)�����ɑ��݂��܂��B\n" "�㏑�����܂����H"
    public: static const char *  M42() ; // "�t�@�C��(%s)���o�͂����ɏI�����܂��B"
    public: static const char *  M43() ; // "�t�@�C��(%s)���o��(fopen)�ł��܂���B\n" "�����𒆎~���܂��B"
    public: static const char *  M46() ; // "�m�F�̂��߁A������x���͂��ĉ������B"
    public: static const char *  M48() ; // "�p�X���[�h����v���܂���B\n" "������x�A���͂������Ă��������B"
    public: static const char *  M49() ; // "�p�X���[�h�����͏������L�����Z������܂����B\n�����𒆎~���܂��B"
    public: static const char *  M50() ; // "�L�����̃p�X���[�h�́A\n\n%s\n\n�ɂȂ�܂��B\n" "�L�^���ĉ������B"
    public: static const char *  M51() ; // "���k���̃�����(%s byte)���m�ۂł��܂���B\n"  "�����𒆎~���܂��B"
    public: static const char *  M54() ; // "���̓f�[�^�̕΂肪�傫���f�[�^�e�[�u�����쐬�ł��܂���B\n" "���̓f�[�^��ύX���邩�����ɕ����Ă��������B\n" "�����𒆎~���܂��B"
    public: static const char *  M55() ; // "�t�@�C��(%s)���o��(fwrite)�ł��܂���B\n" "�����𒆎~���܂��B"
    public: static const char *  M56() ; // "out name: %s (���k�s��:%s, %s byte)"
    public: static const char *  M57() ; // "���̓f�[�^�����Ȃ����f�[�^�����G�Ȃ��߈��k�ł��܂���ł����B\n" "���k�t�@�C��(%s)��\n" "�T�C�Y�͓��̓f�[�^���傫���Ȃ�܂��B�t�@�C�����o�͂��܂����H�B"
    public: static const char *  M59() ; // "���k�������I�����܂����B(�������ԁF%s �b)"
    public: static const char *  M70() ; // "��r�t�@�C���̓��e�ƈ��k�t�@�C���̓��e���s��v�ł��B\n" "��r�t�@�C��(%s)�͑��݂��܂���B\n" "��r�����𑱂��܂����H"
    public: static const char *  M71() ; // "��r�t�@�C���̓��e�ƈ��k�t�@�C���̓��e���s��v�ł��B\n" "��r�t�@�C��(%s)�̃T�C�Y�ƈ��k�t�@�C�����̃t�@�C���T�C�Y\n" "(%s byte, %s byte)���قȂ�܂��B\n" "��r�����𑱂��܂����H"
    public: static const char *  M72() ; // "��r�t�@�C���̓��e�ƈ��k�t�@�C���̓��e���s��v�ł��B\n" "��r�t�@�C��(%s)�̍X�V���t���قȂ�܂��B\n" "��r�t�@�C���F%s" "���k�t�@�C���F%s" "��r�����𑱂��܂����H"
    public: static const char *  M73() ; // "��r�p�̃�����(%s byte)���m�ۂł��܂���B\n" "�����𒆎~���܂��B"
    public: static const char *  M76() ; // "��r�t�@�C���̓��e�ƈ��k�t�@�C���̓��e���s��v�ł��B\n" "��r�t�@�C��(%s)�� %s byte �ڂ̃f�[�^����v���Ȃ��B\n" "%s\n" "��r�����𑱂��܂����H"
    public: static const char *  M77() ; // "�f�B���N�g��(%s)���쐬(mkdir)�ł��܂���B\n" "�����𒆎~���܂��B"
    public: static const char *  M80() ; // "�t�@�C��(%s)�̍X�V���t���ݒ�(utime)�ł��܂���B\n" "�����͑��s���܂��B"
    public: static const char *  M90() ; // "�w�b�_�p�̃�����(%s byte)���m�ۂł��܂���B\n" "�����𒆎~���܂��B"
    public: static const char *  M91() ; // "�t�@�C��(%s)������(stat)�ł��܂���B\n" "�����𒆎~���܂��B"
    // kinaba
    public: static const char *  M92() ; // "�w�b�_�����Ă��܂��B\n" "�����𒆎~���܂��B"
} ;

#define YZ_MSG_TBL_01 (YzMessageTable::M01()) // "�𓀃t�@�C���̏o�̓t�H���_��I��ł��������B"
#define YZ_MSG_TBL_02 (YzMessageTable::M02()) // "       : A> yzdec FileName.yz1           (�t�@�C��'FileName.yz1'����)\n"
#define YZ_MSG_TBL_10 (YzMessageTable::M10()) // "���k�t�@�C���̏o�̓t�H���_��I��ł��������B"
#define YZ_MSG_TBL_11 (YzMessageTable::M11()) // "       : A> yzenc FileName.txt           (�t�@�C��'FileName.txt'�����k)\n"
#define YZ_MSG_TBL_19 (YzMessageTable::M19()) // "�𓀃e�X�g���I�����܂����B\n��肠��܂���B(�������ԁF%s �b)"
#define YZ_MSG_TBL_20 (YzMessageTable::M20()) // "�t�@�C��(%s)������(fopen)�ł��܂���B\n"  "�����𒆎~���܂��B"
#define YZ_MSG_TBL_21 (YzMessageTable::M21()) // "�t�@�C��(%s)������(fread)�ł��܂���B\n"  "�����𒆎~���܂��B"
#define YZ_MSG_TBL_22 (YzMessageTable::M22()) // "�t�@�C��(%s)�͂x�y���k����Ă���t�@�C���ł͂���܂���B\n" "�𓀏����𒆎~���܂��B"
#define YZ_MSG_TBL_23 (YzMessageTable::M23()) // "�p�X���[�h����͂��ĉ������B"
#define YZ_MSG_TBL_24 (YzMessageTable::M24()) // "�p�X���[�h�����͂���܂���ł����B\n�����𒆎~���܂��B"
#define YZ_MSG_TBL_25 (YzMessageTable::M25()) // "�p�X���[�h����v���܂���B\n�����𒆎~���܂��B"
#define YZ_MSG_TBL_26 (YzMessageTable::M26()) // "�𓀐�̃�����(%s byte)���m�ۂł��܂���B\n"  "�����𒆎~���܂��B"
#define YZ_MSG_TBL_27 (YzMessageTable::M27()) // "�t�@�C��(%s file)�����ɑ��݂��܂��B\n" "�㏑�����܂����H"
#define YZ_MSG_TBL_28 (YzMessageTable::M28()) // "�t�@�C���̓��e�ƈ��k�t�@�C���̓��e���r���܂����H"
#define YZ_MSG_TBL_29 (YzMessageTable::M29()) // "�t�@�C�����o�͂����ɏI�����܂��B"
#define YZ_MSG_TBL_31 (YzMessageTable::M31()) // "���̓f�[�^���ُ�ł���B\n"  "���̓f�[�^���ɃG���A�𒴂��ĉ𓀂��悤�Ƃ���R�[�h������B\n" "�t�@�C�����r�����ɏI�����܂��B"
#define YZ_MSG_TBL_32 (YzMessageTable::M32()) // "���̓f�[�^���ُ�ł���B\n" "���̓f�[�^���ɑ��݂��Ȃ��f�[�^�������R�[�h������B\n" "�t�@�C�����r�����ɏI�����܂��B"
#define YZ_MSG_TBL_33 (YzMessageTable::M33()) // "��r�������I�����܂����B\n�t�@�C���̓��e�ƈ�v���܂��B(�������ԁF%s �b)"
#define YZ_MSG_TBL_34 (YzMessageTable::M34()) // "��r�������I�����܂����B\n�t�@�C���̓��e�ƈ�v���܂���ł����B(�������ԁF%s �b)"
#define YZ_MSG_TBL_38 (YzMessageTable::M38()) // "�L���������I�����܂����B(�������ԁF%s �b)"
#define YZ_MSG_TBL_39 (YzMessageTable::M39()) // "�g���q�� '" YZ_EXT1 "' �̃t�@�C��(%s%s)�͏����ł��܂���B\n" "�g���q��ύX���ĉ������B"
#define YZ_MSG_TBL_40 (YzMessageTable::M40()) // "���̓t�@�C��(%s%s)�Ɠ����t�@�C���ɏo�͂ł��܂���B\n" "�t�@�C���̏o�͐��ύX���ĉ������B"
#define YZ_MSG_TBL_41 (YzMessageTable::M41()) // "�o�̓t�@�C��(%s)�����ɑ��݂��܂��B\n" "�㏑�����܂����H"
#define YZ_MSG_TBL_42 (YzMessageTable::M42()) // "�t�@�C��(%s)���o�͂����ɏI�����܂��B"
#define YZ_MSG_TBL_43 (YzMessageTable::M43()) // "�t�@�C��(%s)���o��(fopen)�ł��܂���B\n" "�����𒆎~���܂��B"
#define YZ_MSG_TBL_46 (YzMessageTable::M46()) // "�m�F�̂��߁A������x���͂��ĉ������B"
#define YZ_MSG_TBL_48 (YzMessageTable::M48()) // "�p�X���[�h����v���܂���B\n" "������x�A���͂������Ă��������B"
#define YZ_MSG_TBL_49 (YzMessageTable::M49()) // "�p�X���[�h�����͏������L�����Z������܂����B\n�����𒆎~���܂��B"
#define YZ_MSG_TBL_50 (YzMessageTable::M50()) // "�L�����̃p�X���[�h�́A\n\n%s\n\n�ɂȂ�܂��B\n" "�L�^���ĉ������B"
#define YZ_MSG_TBL_51 (YzMessageTable::M51()) // "���k���̃�����(%s byte)���m�ۂł��܂���B\n"  "�����𒆎~���܂��B"
#define YZ_MSG_TBL_54 (YzMessageTable::M54()) // "���̓f�[�^�̕΂肪�傫���f�[�^�e�[�u�����쐬�ł��܂���B\n" "���̓f�[�^��ύX���邩�����ɕ����Ă��������B\n" "�����𒆎~���܂��B"
#define YZ_MSG_TBL_55 (YzMessageTable::M55()) // "�t�@�C��(%s)���o��(fwrite)�ł��܂���B\n" "�����𒆎~���܂��B"
#define YZ_MSG_TBL_56 (YzMessageTable::M56()) // "out name: %s (���k�s��:%s, %s byte)"
#define YZ_MSG_TBL_57 (YzMessageTable::M57()) // "���̓f�[�^�����Ȃ����f�[�^�����G�Ȃ��߈��k�ł��܂���ł����B\n" "���k�t�@�C��(%s)��\n" "�T�C�Y�͓��̓f�[�^���傫���Ȃ�܂��B�t�@�C�����o�͂��܂����H�B"
#define YZ_MSG_TBL_59 (YzMessageTable::M59()) // "���k�������I�����܂����B(�������ԁF%s �b)"
#define YZ_MSG_TBL_70 (YzMessageTable::M70()) // "��r�t�@�C���̓��e�ƈ��k�t�@�C���̓��e���s��v�ł��B\n" "��r�t�@�C��(%s)�͑��݂��܂���B\n" "��r�����𑱂��܂����H"
#define YZ_MSG_TBL_71 (YzMessageTable::M71()) // "��r�t�@�C���̓��e�ƈ��k�t�@�C���̓��e���s��v�ł��B\n" "��r�t�@�C��(%s)�̃T�C�Y�ƈ��k�t�@�C�����̃t�@�C���T�C�Y\n" "(%s byte, %s byte)���قȂ�܂��B\n" "��r�����𑱂��܂����H"
#define YZ_MSG_TBL_72 (YzMessageTable::M72()) // "��r�t�@�C���̓��e�ƈ��k�t�@�C���̓��e���s��v�ł��B\n" "��r�t�@�C��(%s)�̍X�V���t���قȂ�܂��B\n" "��r�t�@�C���F%s" "���k�t�@�C���F%s" "��r�����𑱂��܂����H"
#define YZ_MSG_TBL_73 (YzMessageTable::M73()) // "��r�p�̃�����(%s byte)���m�ۂł��܂���B\n" "�����𒆎~���܂��B"
#define YZ_MSG_TBL_76 (YzMessageTable::M76()) // "��r�t�@�C���̓��e�ƈ��k�t�@�C���̓��e���s��v�ł��B\n" "��r�t�@�C��(%s)�� %s byte �ڂ̃f�[�^����v���Ȃ��B\n" "%s\n" "��r�����𑱂��܂����H"
#define YZ_MSG_TBL_77 (YzMessageTable::M77()) // "�f�B���N�g��(%s)���쐬(mkdir)�ł��܂���B\n" "�����𒆎~���܂��B"
#define YZ_MSG_TBL_80 (YzMessageTable::M80()) // "�t�@�C��(%s)�̍X�V���t���ݒ�(utime)�ł��܂���B\n" "�����͑��s���܂��B"
#define YZ_MSG_TBL_90 (YzMessageTable::M90()) // "�w�b�_�p�̃�����(%s byte)���m�ۂł��܂���B\n" "�����𒆎~���܂��B"
#define YZ_MSG_TBL_91 (YzMessageTable::M91()) // "�t�@�C��(%s)������(stat)�ł��܂���B\n" "�����𒆎~���܂��B"
// kinaba
#define YZ_MSG_TBL_92 (YzMessageTable::M92()) // "�w�b�_�����Ă��܂��B\n" "�����𒆎~���܂��B"


#define YZ_MSG_NO_01 (   1) // "�𓀃t�@�C���̏o�̓t�H���_��I��ł��������B"
#define YZ_MSG_NO_02 (   2) // "       : A> yzdec file.yz1           (�t�@�C��'file.yz1'����)\n"
#define YZ_MSG_NO_10 (  10) // "���k�t�@�C���̏o�̓t�H���_��I��ł��������B"
#define YZ_MSG_NO_11 (  11) // "       : A> yzenc file.txt           (�t�@�C��'file.txt'�����k)\n"
#define YZ_MSG_NO_19 (1019) // MessINF "�𓀃e�X�g���I�����܂����B\n��肠��܂���B(�������ԁF%s �b)"
#define YZ_MSG_NO_20 (2020) // MessEX "�t�@�C��(%s)������(fopen)�ł��܂���B\n"  "�����𒆎~���܂��B"
#define YZ_MSG_NO_21 (2021) // MessEX "�t�@�C��(%s)������(fread)�ł��܂���B\n"  "�����𒆎~���܂��B"
#define YZ_MSG_NO_22 (2022) // MessEX "�t�@�C��(%s)�͂x�y���k����Ă���t�@�C���ł͂���܂���B\n" "�𓀏����𒆎~���܂��B"
#define YZ_MSG_NO_23 (3023) // MessINPUT "�p�X���[�h����͂��ĉ������B"
#define YZ_MSG_NO_24 (1024) // MessINF "�p�X���[�h�����͂���܂���ł����B\n�����𒆎~���܂��B"
#define YZ_MSG_NO_25 (1025) // MessINF "�p�X���[�h����v���܂���B\n�����𒆎~���܂��B"
#define YZ_MSG_NO_26 (2026) // MessEX "�𓀐�̃�����(%s byte)���m�ۂł��܂���B\n"  "�����𒆎~���܂��B"
#define YZ_MSG_NO_27 (4027) // MessYN "�t�@�C��(%s file)�����ɑ��݂��܂��B\n" "�㏑�����܂����H"
#define YZ_MSG_NO_28 (4028) // MessYN "�t�@�C���̓��e�ƈ��k�t�@�C���̓��e���r���܂����H"
#define YZ_MSG_NO_29 (1029) // MessINF "�t�@�C�����o�͂����ɏI�����܂��B"
#define YZ_MSG_NO_31 (2031) // MessEX "���̓f�[�^���ُ�ł���B\n"  "���̓f�[�^���ɃG���A�𒴂��ĉ𓀂��悤�Ƃ���R�[�h������B\n" "�t�@�C�����r�����ɏI�����܂��B"
#define YZ_MSG_NO_32 (2032) // MessEX "���̓f�[�^���ُ�ł���B\n" "���̓f�[�^���ɑ��݂��Ȃ��f�[�^�������R�[�h������B\n" "�t�@�C�����r�����ɏI�����܂��B"
#define YZ_MSG_NO_33 (1033) // MessINF "��r�������I�����܂����B\n�t�@�C���̓��e�ƈ�v���܂��B(�������ԁF%s �b)"
#define YZ_MSG_NO_34 (1034) // MessINF "��r�������I�����܂����B\n�t�@�C���̓��e�ƈ�v���܂���ł����B(�������ԁF%s �b)"
#define YZ_MSG_NO_38 (1038) // MessINF "�L���������I�����܂����B(�������ԁF%s �b)"
#define YZ_MSG_NO_39 (2039) // MessEX "�g���q�� '" YZ_EXT1 "' �̃t�@�C��(%s%s)�͏����ł��܂���B\n" "�g���q��ύX���ĉ������B"
#define YZ_MSG_NO_40 (2040) // MessEX "���̓t�@�C��(%s%s)�Ɠ����t�@�C���ɏo�͂ł��܂���B\n" "�t�@�C���̏o�͐��ύX���ĉ������B"
#define YZ_MSG_NO_41 (4041) // MessYN "�o�̓t�@�C��(%s)�����ɑ��݂��܂��B\n" "�㏑�����܂����H"
#define YZ_MSG_NO_42 (1042) // MessINF "�t�@�C��(%s)���o�͂����ɏI�����܂��B"
#define YZ_MSG_NO_43 (2043) // MessEX "�t�@�C��(%s)���o��(fopen)�ł��܂���B\n" "�����𒆎~���܂��B"
#define YZ_MSG_NO_46 (3046) // MessINPUT "�m�F�̂��߁A������x���͂��ĉ������B"
#define YZ_MSG_NO_48 (4048) // MessYN "�p�X���[�h����v���܂���B\n" "������x�A���͂������Ă��������B"
#define YZ_MSG_NO_49 (1049) // MessINF "�p�X���[�h�����͏������L�����Z������܂����B\n�����𒆎~���܂��B"
#define YZ_MSG_NO_50 (1050) // MessINF "�L�����̃p�X���[�h�́A\n\n%s\n\n�ɂȂ�܂��B\n" "�L�^���ĉ������B"
#define YZ_MSG_NO_51 (2051) // MessEX "���k���̃�����(%s byte)���m�ۂł��܂���B\n"  "�����𒆎~���܂��B"
#define YZ_MSG_NO_54 (2054) // MessEX "���̓f�[�^�̕΂肪�傫���f�[�^�e�[�u�����쐬�ł��܂���B\n" "���̓f�[�^��ύX���邩�����ɕ����Ă��������B\n" "�����𒆎~���܂��B"
#define YZ_MSG_NO_55 (2055) // MessEX "�t�@�C��(%s)���o��(fwrite)�ł��܂���B\n" "�����𒆎~���܂��B"
#define YZ_MSG_NO_56 (  56) // "out name: %s (���k�s��:%s, %s byte)"
#define YZ_MSG_NO_57 (4057) // MessYN "���̓f�[�^�����Ȃ����f�[�^�����G�Ȃ��߈��k�ł��܂���ł����B\n" "���k�t�@�C��(%s)��\n" "�T�C�Y�͓��̓f�[�^���傫���Ȃ�܂��B�t�@�C�����o�͂��܂����H�B"
#define YZ_MSG_NO_59 (1059) // MessINF "���k�������I�����܂����B(�������ԁF%s �b)"
#define YZ_MSG_NO_70 (4070) // MessYN "��r�t�@�C���̓��e�ƈ��k�t�@�C���̓��e���s��v�ł��B\n" "��r�t�@�C��(%s)�͑��݂��܂���B\n" "��r�����𑱂��܂����H"
#define YZ_MSG_NO_71 (4071) // MessYN "��r�t�@�C���̓��e�ƈ��k�t�@�C���̓��e���s��v�ł��B\n" "��r�t�@�C��(%s)�̃T�C�Y�ƈ��k�t�@�C�����̃t�@�C���T�C�Y\n" "(%s byte, %s byte)���قȂ�܂��B\n" "��r�����𑱂��܂����H"
#define YZ_MSG_NO_72 (4072) // MessYN "��r�t�@�C���̓��e�ƈ��k�t�@�C���̓��e���s��v�ł��B\n" "��r�t�@�C��(%s)�̍X�V���t���قȂ�܂��B\n" "��r�t�@�C���F%s" "���k�t�@�C���F%s" "��r�����𑱂��܂����H"
#define YZ_MSG_NO_73 (2073) // MessEX "��r�p�̃�����(%s byte)���m�ۂł��܂���B\n" "�����𒆎~���܂��B"
#define YZ_MSG_NO_76 (4076) // MessYN "��r�t�@�C���̓��e�ƈ��k�t�@�C���̓��e���s��v�ł��B\n" "��r�t�@�C��(%s)�� %s byte �ڂ̃f�[�^����v���Ȃ��B\n" "%s\n" "��r�����𑱂��܂����H"
#define YZ_MSG_NO_77 (2077) // MessEX "�f�B���N�g��(%s)���쐬(mkdir)�ł��܂���B\n" "�����𒆎~���܂��B"
#define YZ_MSG_NO_80 (1080) // MessINF "�t�@�C��(%s)�̍X�V���t���ݒ�(utime)�ł��܂���B\n" "�����͑��s���܂��B"
#define YZ_MSG_NO_90 (2090) // MessEX "�w�b�_�p�̃�����(%s byte)���m�ۂł��܂���B\n" "�����𒆎~���܂��B"
#define YZ_MSG_NO_91 (2091) // MessEX "�t�@�C��(%s)������(stat)�ł��܂���B\n" "�����𒆎~���܂��B"
// kinaba
#define YZ_MSG_NO_92 (2092) // MessEX "�w�b�_�����Ă��܂��B\n" "�����𒆎~���܂��B"


#endif // __YZ_MagTbl_h__
