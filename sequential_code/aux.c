#include "projet.h"

/* 2017-02-23 : version 1.0 */

typedef struct {
  unsigned long long hash;
  int flag;
  int depth;
  int score;
  int best_move;
} tt_entry_t;

tt_entry_t *tt;
unsigned long long int tt_probe, tt_hit, tt_partial;



#define WHITE 1
#define BLACK 0

#define SQUARE(rank,file) ((file) +((rank) <<4) )
#define SQ_FILE(sq) ((sq) &0x07)
#define SQ_RANK(sq) ((sq) >>4)
#define VALID(sq) (((sq) &0x88) ==0)  
#define NORTH 16
#define NN (NORTH+NORTH)
#define SOUTH -16
#define SS (SOUTH+SOUTH)
#define EAST 1
#define WEST -1
#define NE 17
#define SW -17
#define NW 15
#define SE -15

#define MOVE(from,to) ((from) +((to) <<8) )
#define MV_FROM(move) ((move) &0xff)
#define MV_TO(move) ((move) >>8) 
#define KING 0
#define PAWN 1

#define CAPTURE(move) (T->pieces[MV_TO(move) ]>=0)
#define PAWN_MOVE(move) (T->pieces[MV_FROM(move) ]==PAWN)

#define PAWN_ATTACKS(side,sq) (T->attack[sq]&(2*(1+15*(side) ) ) )
#define KING_ATTACKS(side,sq) (T->attack[sq]&(1*(1+15*(side) ) ) )
#define BAD_MOVE 0xdead

// #define FINALIZE(x) ((x==CERTAIN_DRAW) ?CERTAIN_DRAW:((x> 0) ?x-1:x+1) )
#define ENQUEUE_MOVE(target) {if(VALID(target) ) moves[n_moves++]= MOVE(sq,target) ;};
#define EMPTY(sq) (T->pieces[sq]<0)
#define ENEMY(sq) (T->colors[sq]==1-T->side)
#define FRIENDLY(sq) (T->colors[sq]==T->side)
 
#define TT_INVALID -1
#define TT_EXACT 0
#define TT_LOWER_BOUND 1
#define TT_UPPER_BOUND 2
#define TT_SIZE 10000019

#define ATTACKS(side,sq) (T->attack[sq]&(3*(1+15*(side) ) ) )

int pawn_move[2] = { SOUTH, NORTH };

int n_attack_vectors[2] = { 8, 2 };

square_t attack_vectors[2][2][8] = {
	{{NE, NORTH, NW, EAST, WEST, SE, SOUTH, SW},
	 {NE, NORTH, NW, EAST, WEST, SE, SOUTH, SW}},
	{{SE, SW}, {NE, NW}}
};


unsigned long long int zobrist_side = 0x18e2b39a2ebedcfd;
unsigned long long int zobrist[128][2][2] = {
	{{0x25bb7fd850b1a1cf, 0x6760bb6c67b844cd},
	 {0x925c183e7d09df65, 0x4b333538418f3c16}},
	{{0xa0e9ec7a28f8a9af, 0xa63374e2d62e419},
	 {0xf0e12f7e6166cb1c, 0xe578d4875ee0cf8c}},
	{{0x1765687950830890, 0xaaf085e317d93bc4},
	 {0xd748b2d4025deab6, 0x9f58a6527961206f}},
	{{0x65394328f6d1c6f9, 0xe8771c19973b874},
	 {0xf11cdd287791f8d5, 0xd48ab36400c40c4b}},
	{{0x82d71b9a4dbcaee1, 0x323db7ae05574574},
	 {0xf3cff7942d18c2bc, 0xe298acca3d22bb49}},
	{{0x998485c7545f9395, 0xfe2c705a8b6c2b19},
	 {0x1cace68e1410b129, 0x9f6b6a1f2dc216da}},
	{{0x5c3cb2225321fb88, 0x796b85bab4fba70f},
	 {0x1a9efe9de73b7a71, 0xe45d3a396ccca07a}},
	{{0xaa9528a6caea2fe3, 0x2fe743fac55e198c},
	 {0x3f3e706fcbcf3ca1, 0xaa2bdd8171c1a5a0}},
	{{0xbe152d60f73a01b9, 0xea63657ee3d860dc},
	 {0x22252f716c3e6b47, 0xadc5982c43cbe86a}},
	{{0xebf5281667652804, 0x489c2dc4eed67e1e},
	 {0x386fba53beec9a30, 0x906c16fb688bea44}},
	{{0x99b844309a2996cb, 0x4f4fab94e10786c3},
	 {0x9ff9029457c0edcb, 0x8956261f811b6ed5}},
	{{0x867eb88960f148dc, 0xd005396b077b7504},
	 {0xd446cf269594e4fa, 0x1d46d10a3e419e84}},
	{{0xe356eca43516a288, 0x9a5b560dffefed17},
	 {0xa4a2aa6a60bc0b96, 0x9d2429881b7c027c}},
	{{0x8664dbd1b472ae5f, 0x587113cb3efac2f5},
	 {0x961adf0119244afa, 0xe0f7f1e6f4ecf162}},
	{{0xc6cbb848e117b149, 0x886010af1ad344ca},
	 {0xef67491a6f80dcd7, 0x15a898e75c4464ea}},
	{{0x696b7008836d555d, 0xb039be1f6feedf0},
	 {0x21c940e82bbdbf57, 0x7d9421861b372a56}},
	{{0x4bf7dd0110a6950b, 0x5b8d350a8b779a10},
	 {0x6c02a03f8549afed, 0xc55c4575266db5c}},
	{{0xa9102bc1cc367f78, 0x4a242d108e260056},
	 {0xfb7f008ae0213976, 0x8e7d1a541314bec2}},
	{{0x1b01c15d17077ef5, 0xbe2faa7252379a6d},
	 {0xf1e7d56de92a2959, 0xc56694c4eeaea44e}},
	{{0xa006dac0b6689574, 0x648dc3726643c7da},
	 {0x9fcb1b811333dc08, 0xb4d584d818c92e67}},
	{{0xa88fe054467179f7, 0x3eba0ab917c7cc94},
	 {0x1a7bc38d31d66762, 0xdd28b958c03bae66}},
	{{0x39af24da6e775bdf, 0xc18cf8503affb1af},
	 {0xdc57b90a513883f1, 0x65f8bbebbd80a938}},
	{{0xb854dcc469e111bc, 0xe06e457542211660},
	 {0xed5b8dc4846d4dc0, 0x7cf61bce6d78b3a1}},
	{{0x1eab647fa4ba93ac, 0x8c9ed33f7a3ae80c},
	 {0x9fd9af72b4bdec68, 0xd99a31fca85a34ea}},
	{{0x599ab319166904b, 0xe60c86569f607af9},
	 {0x1fa64a4c8b434b4a, 0x30e01d8ad4704722}},
	{{0x8779c935bfbf5a75, 0xcb7f5b77f38c0baf},
	 {0x611ae5fb14ea8d30, 0x71329ebaa06c87d9}},
	{{0x1ec8ccd65e3f23d9, 0x8a297506f8c1c3fa},
	 {0x17b6364a8d943cd2, 0x93b61010556a6c8c}},
	{{0x6921e4cb4f6f4bdb, 0xe57de297266a0ae1},
	 {0xab316697d3e365f0, 0xd5f33f40cb74a85d}},
	{{0x3f650a5e95413770, 0xb1764f71c7d813fa},
	 {0xaa6c5ba367e957cf, 0x3d8c4c6e25dbc852}},
	{{0xe26db51ca5dfc8ca, 0x55f7a76c3a4b5fe1},
	 {0x7b7c22ac3d78e392, 0x576d43abe2eec7a7}},
	{{0x8f4596d563f1d430, 0xf51aec6578f58b94},
	 {0xc52a6bb9ee6cc2d4, 0xf83bb2f7eb1c8866}},
	{{0x99e23559354900b, 0x5b8da52fa9b35499},
	 {0xf883a732bb8fd1b5, 0x6a2ecb4ce3b4da63}},
	{{0x791b219241f3bed0, 0xffd7c92168fc1402},
	 {0xfcc33a81d6f6cc40, 0xd66ff037a9f39e90}},
	{{0x54340c67407458dd, 0x52beae3aaa956e3},
	 {0x8facaaa1a352d40a, 0xdbc7a65289087ef0}},
	{{0xf8fa92bac23a1132, 0x144fe04c8438c4ba},
	 {0xc1a00f33d3507b72, 0x4507d07a31e87183}},
	{{0x1976db0a18891251, 0x4b9ef18eedad06c7},
	 {0x7e0a8d5beb7c7c65, 0xc666c2f75f8a7b7d}},
	{{0x343a85afaaa09cd0, 0xe26ac73437af655c},
	 {0x9b5e17a0fa68fbb, 0xb0f9127646cdcf92}},
	{{0xf1e9ea54a94897e8, 0xb0e5d7e8a121d68c},
	 {0x5b63d245ef954c51, 0x28a885306d83adc9}},
	{{0x58cc79a39ec918ac, 0xa48a57649edbf0ab},
	 {0x6e2a1d045c7eb114, 0xd918159c62b932db}},
	{{0xda87546a69de1d71, 0xce8e9057874be238},
	 {0x4f469cec30fa2546, 0x9defba689666159c}},
	{{0xf9dbb8a681298071, 0xd1b75340099a1075},
	 {0x92abc38eeaa9d90, 0xbdeb36295efbf809}},
	{{0x6744ba87696ca1a1, 0x5d1b8716239b45a5},
	 {0xe798d5221a80b384, 0x1c4ba90d2a2c4a36}},
	{{0xfc67eaa5be43705e, 0x73292504eb12b993},
	 {0x7851baf946185e7a, 0x5eca7c473aa67084}},
	{{0xad5ba47a028e7fe9, 0xfed49ed054b0d284},
	 {0x92c2850f71bf223e, 0x6b1ba8e43ce981eb}},
	{{0x63305ea26a2fc82f, 0x8844e4b55d6d8cf5},
	 {0xb6f4eaf05c3eb75a, 0x3c12dd0ce258f5b4}},
	{{0xe9e3de4cf4613572, 0x8f89aa914aa5e2be},
	 {0x7b93eb9fda8b03f4, 0x19853ea86cc89efb}},
	{{0x213ae3cfb87986d8, 0xc37ba482a9b176ee},
	 {0x388afeac83566bd, 0x7b7fb197b863a2e5}},
	{{0xdb3976e37de2350a, 0xd0f9484ab325d1fc},
	 {0x17d4135dbc697272, 0x9af5891fa6583b3}},
	{{0x4b028f03261b190d, 0x6cc475922882052d},
	 {0x9ced282d7f33e699, 0x676623e1084a4ced}},
	{{0x64f7da129cb3f488, 0x175a3d8e88220c88},
	 {0x4ffa62fa8644c80e, 0x5805e17e406c5779}},
	{{0xcabe56f2562c1acc, 0x20d2cd410aecfd55},
	 {0x7adc51d6f238438c, 0xb9abb22fde2435b}},
	{{0x1626f9e5453e4bc0, 0xa0d46a51ad0863c0},
	 {0x8468ad5c3f50132d, 0xf8efbd228589085b}},
	{{0x7aba83d7710f9703, 0x44d3f89ea69d5b8e},
	 {0x81c2dbbcdcf1fc6a, 0xf2c6ebe7ab5e238}},
	{{0xaf5829b27dd9fd79, 0x4cc617ba64b10180},
	 {0xa2aba94857c97aa, 0x653f7e5e6d7e3ce0}},
	{{0x53c053fc83f80fb6, 0x18ed747d27bb8401},
	 {0xde386bd698916d6b, 0xdf88d2e4984bdc08}},
	{{0xb28500fd1c0dd684, 0x1ce6c8152dad5ce2},
	 {0x89000c254f4d8d42, 0x4c24e2970b7f17c2}},
	{{0x6b3bd34e491bbb7e, 0x2f066d732d0de7bc},
	 {0x2bfd394530be3d24, 0x8ea65ff964fe2ee7}},
	{{0x9e083ecf407d77eb, 0xee07b260fa2fba7a},
	 {0x1c4acdea4502f828, 0xe0836e2422b714cb}},
	{{0x542c7d52906239ee, 0xba71f866279384c2},
	 {0x2455c7f999efff62, 0x6ab893d5917b8d60}},
	{{0x6e805d087894e299, 0xfa14d1886a567da2},
	 {0x94f4ba00d0ced510, 0x2b2b40e26294c04c}},
	{{0xd7445dfe0b9183c, 0x4a3e0aff9ff5e1fa},
	 {0x298ee698efc7540b, 0xbafdb308bf7b62a5}},
	{{0xb5dfcba610d59d2, 0x3df24c5b8eb5b46a},
	 {0xadcf9ef53ada7b4a, 0xc9fa3118aec4e96f}},
	{{0xc06a0a95d1831c33, 0xbd1900f86d951295},
	 {0xde92d40857a7c2fa, 0x262c08ad2cf5714f}},
	{{0xd81af1d59f1b480e, 0x954a799a073e70be},
	 {0xe2df609d4c8396f0, 0x70631dde0da5df30}},
	{{0x60702ac5d4ca90b0, 0x29e0727f2e835d83},
	 {0xec7ebf3c1ba17a6b, 0x14233250616e2aba}},
	{{0xc8b7925449699ba2, 0x685ffe571f53a686},
	 {0xab1fb295268af924, 0xf96452e646013e59}},
	{{0xc8d45d4d917ce9f8, 0x5840edae8e56036d},
	 {0x39123b7fd4039fad, 0xc6ffc3ea0a821fa2}},
	{{0x97c67d6f1f31e8e9, 0x8a1d771c9a847646},
	 {0xea660af53fd7dff3, 0x24cbe364e82c46a8}},
	{{0xfd598117ab879799, 0x19d74d68fc7877e7},
	 {0x988e2e799bad357f, 0xbfbf9308ae2e96a9}},
	{{0xab1857b87bb19ed9, 0x631141d4d035422a},
	 {0x7df52aad8d4a66b, 0xcd11931bbb248b3f}},
	{{0x3765377fc4f0e764, 0x789fd75777b647},
	 {0x499b0fc5054dc1bd, 0xb6737a32b22dd411}},
	{{0x2ac5866b769b4698, 0x80bca49d83f50c05},
	 {0x5f6bb28884768578, 0xaf0cc7851cf6e1f6}},
	{{0x650df54eb2614aa2, 0x423a81b62d4a80b9},
	 {0x18d18453863fc977, 0xe20ed15e87ccc2d6}},
	{{0x25d694be792bc218, 0x1d4049ecc4a38d93},
	 {0xeb92fd55a348df5d, 0x5017e844187edbdc}},
	{{0x506f6cc8b1b9eb8e, 0x488a16712888ecbf},
	 {0xdf033f2d7629a1d8, 0x52f16b13964188d1}},
	{{0x1e775abd07c7ac57, 0xe264a0db5d16d9b7},
	 {0x55c0aed36220ccaa, 0xbd8b826170b96cf9}},
	{{0x6f83bcc82777d16f, 0xa1578c9078cc3f08},
	 {0xa179337f0a98f29b, 0xa754cb4ce9a706cf}},
	{{0xf4f6ff72f3e994e7, 0x21b3a3d4047beed7},
	 {0xad31ee07f017d64e, 0x295a382f670e36ce}},
	{{0xadf2724173f3ab6e, 0xf0eefc9ec5d7b9f5},
	 {0x608e1ae812e5e8e3, 0x3125cf7578b0639a}},
	{{0xed03d3a380b79f1f, 0x82f4d196fef57916},
	 {0x5f50b45f204b49c7, 0xc17f99828f34d41c}},
	{{0xa1603fa1c1bb77e4, 0xa59a4a1e1b805692},
	 {0x87061d34f2ddf270, 0x70c0d0da23363f38}},
	{{0xdab0d1535c199f13, 0x5b56050fc465f423},
	 {0x25d3ec4684e2289b, 0xed1b878ff94dc7f}},
	{{0x3d0e9dbef8067dcb, 0x290d98f33f2699de},
	 {0xdfe9a3252c7966dc, 0xfafe9f9ef85dbcc}},
	{{0xabde30c1a529a7d, 0x562315c1dda6646a},
	 {0xd1a2cd903d428f81, 0xc10e5183a580cd91}},
	{{0x7b963325041dccbf, 0x900333e2b9126186},
	 {0x7ea34e45afb7732c, 0xe7920860f9f44220}},
	{{0xa03d42c457104e15, 0x8f69507fbe077891},
	 {0x654835be4f8bfec5, 0xf1f8833435d724e5}},
	{{0x64cd3cf5569dea7c, 0x4ad195c481f708ab},
	 {0x9d86d34c375184f, 0x5eff64ec699db8d8}},
	{{0xab067d7aba63fded, 0xa5893f6bbddd3f84},
	 {0x4b676830ef84f59f, 0x9c8b0bb395b631f0}},
	{{0xa70efb6dc549916f, 0x86c40a1a62ab460a},
	 {0x849c526bf99a71ca, 0x627d593fb71304b2}},
	{{0x6ff47cbf68e81dc9, 0xefe51e636e88bb7c},
	 {0x80bb71e7b6cf23b4, 0x5d5d6829a0b80746}},
	{{0x61d847c4380eeaf5, 0x946ac71edc8625df},
	 {0x4a0a59cb4498570, 0xde6a183941c050b}},
	{{0xd5c29e0ea1ad12a4, 0x2986f8836163f273},
	 {0x4bb455b8cf06c094, 0x8ba353bc96e6034b}},
	{{0x2dbee1d93844f3c, 0x6c2707b625d42e91},
	 {0x5fa2b9673f4f2dc8, 0x72b47c422b2b8fb1}},
	{{0x365c6164bff82cc7, 0xab36d5e5e0e76573},
	 {0xe7f77f328ea12088, 0x32e249d10a6dff0d}},
	{{0x64752b8a104bf583, 0x2f93cd891c2030e0},
	 {0x98a2c84221bc28e7, 0xaf68748c705616d3}},
	{{0x7a02b7cc8fa092df, 0x83c4b7bd2ad0b9bb},
	 {0xf1b6a396437e1990, 0xb0a6237c03c99576}},
	{{0x9d728593a5195268, 0x3a959f75958f9411},
	 {0x76ad703924231af8, 0x3ef858fbc9a2e1bf}},
	{{0x1f465086226c71f4, 0x76005c68e13bdfe2},
	 {0xf4cd54eda294d8ed, 0x5697b48ac036c727}},
	{{0xa12195cc0ad98721, 0x83b8640164cb024d},
	 {0x537b39969c28ae67, 0xffd5bd91f3d13df4}},
	{{0x4152d0daab468867, 0x75620fb0df3bf65a},
	 {0xbe2ae4e2ffe9cf, 0x8c56d55277e8bf0e}},
	{{0xd410efe5aae1bf7e, 0x448311daaa278b37},
	 {0x68f07901e053db10, 0xfe7332d1550b93b9}},
	{{0x62b58dce91e05ba6, 0xe3f897d3562b4607},
	 {0x65be5e4898a95c35, 0x62a500b989549dc1}},
	{{0x16ec1d799101d684, 0x31e097cbe4f04dbb},
	 {0x2eb7b65789abda1f, 0x67a56c93da574822}},
	{{0x2642713ad1615f8a, 0xf45761285acac7d0},
	 {0x639c6307452d1e46, 0x5c30b055cc5089c5}},
	{{0x969e32e6da8e678e, 0x7fae12faef3c5315},
	 {0xa42da88df525f37a, 0x77b057c41597cb1c}},
	{{0xd255759969c2a23e, 0x31a39a69663999dd},
	 {0x79130a89983d75d1, 0xe84ff88471509c3d}},
	{{0x6393e1423ede6b2b, 0xd0bd92123eb36f45},
	 {0x75f6edfe42ff5a5e, 0xe62b1a12d8dd997e}},
	{{0xdf01e0d21f21962f, 0x631c85cf241eb232},
	 {0x58ee9e40d864274d, 0x4c1c72aaf4bee79e}},
	{{0x42592fbab8bd8e56, 0xcef2857a2fcb6818},
	 {0x432461aa39da0b48, 0x2a3a2bfb17dab63e}},
	{{0x2ee160eeb8f6d3ec, 0x5bd1608117b38a85},
	 {0x78ae812cbb3f65c9, 0x9b6006c64805d560}},
	{{0x21223b04957471fd, 0xed72ff18ffbb4862},
	 {0xf99a668c1bfd4f31, 0x33f86fdc979b8049}},
	{{0xc5bf51df3b6a43d6, 0x5502226c7ab6a355},
	 {0x6ef512c41e078ba5, 0x544529697666494c}},
	{{0xa008722bfb984e30, 0x14d7acc07e3178f8},
	 {0xdbca470975fb4f93, 0xa9191a345167335d}},
	{{0xab84fd55942bc4ad, 0x7c812fc0014bcc2a},
	 {0x17df374f1615495d, 0xb925a5d2f5d8ac80}},
	{{0xe71640ed2909cc7b, 0xf3885b26f2765ae7},
	 {0x2aa060f434c5b952, 0x1586cc5e59a40b5f}},
	{{0xe5b753b8eddb6423, 0x1bd3308d669a430c},
	 {0xeada09b81ea4039, 0x9a1e4a8ac1b36d2}},
	{{0xcbf65263533a0899, 0xe93d34d69bc6d5bc},
	 {0x73c7f45be3c9aaf5, 0x3028f2f838535aae}},
	{{0xf3c216f6a678bc28, 0x59969da183237893},
	 {0x52740680b1eeeb35, 0x9bf0518840e44166}},
	{{0x4079c017e3705eec, 0x555117e38896bd37},
	 {0x8b7da4aecd07591d, 0xb8fb518504a13cf4}},
	{{0xed30babb78a5f54f, 0x66cd3d21e3192b0b},
	 {0xb37c3ecd951a28f4, 0x5122e4456b4c46c4}},
	{{0x90cf0e29cdb3c62b, 0xd9a31cfca78b82d1},
	 {0x87d944dc1857f722, 0x89c51116bdecf212}},
	{{0xd505e7dda882bb8e, 0xbc6682e6d2865a42},
	 {0x80ec3469c2e2fa13, 0x56fae4f762afd02b}},
	{{0xe7309455a749133d, 0x21d397a97aa18736},
	 {0x44a6aec14c1b2fd, 0xac9cfcbab2aba2a4}},
	{{0xe1b1c61aa11abd4, 0x7224af67fecb2816},
	 {0x97cefb1f9d3bd9a5, 0xee07ee750bc36ddc}},
	{{0xd989d0a739a33408, 0x6495daa4906fd0ff},
	 {0x66133a161ddaab4b, 0xa2b51470252aedfc}},
	{{0x2ca856256a20e498, 0x9a2f0d730d58be5c},
	 {0xe0e585e9ddfeea0d, 0x9aacc55350bb696}},
	{{0xb28fb810f1d1181d, 0x40232725b01c739b},
	 {0xcd0f5284ae3709, 0xdc9f88c7067d55ff}},
	{{0x9107d0ade3bcc465, 0xbe5561f8689e7742},
	 {0x7bd10f7077c890b7, 0xc1f5514b8ee059d3}}
};


int K_sqpc[2][128] = {
	{
	 -70, -60, -30, -30, -30, -30, -60, -70, 0, 0, 0, 0, 0, 0, 0, 0,
	 -60, -50, -30, -20, -20, -30, -50, -60, 0, 0, 0, 0, 0, 0, 0, 0,
	 -50, -30, 0, 0, 0, 0, -30, -50, 0, 0, 0, 0, 0, 0, 0, 0,
	 -40, -20, 0, 5, 5, 0, -20, -40, 0, 0, 0, 0, 0, 0, 0, 0,
	 -40, -20, 0, 5, 5, 0, -20, -40, 0, 0, 0, 0, 0, 0, 0, 0,
	 -50, -30, 0, 0, 0, 0, -30, -50, 0, 0, 0, 0, 0, 0, 0, 0,
	 -60, -50, -30, -20, -20, -30, -50, -60, 0, 0, 0, 0, 0, 0, 0, 0,
	 -70, -60, -50, -40, -40, -50, -60, -70, 0, 0, 0, 0, 0, 0, 0, 0,},
	{
	 -70, -60, -50, -40, -40, -50, -60, -70, 0, 0, 0, 0, 0, 0, 0, 0,
	 -60, -50, -30, -20, -20, -30, -50, -60, 0, 0, 0, 0, 0, 0, 0, 0,
	 -50, -30, 0, 0, 0, 0, -30, -50, 0, 0, 0, 0, 0, 0, 0, 0,
	 -40, -20, 0, 5, 5, 0, -20, -40, 0, 0, 0, 0, 0, 0, 0, 0,
	 -40, -20, 0, 5, 5, 0, -20, -40, 0, 0, 0, 0, 0, 0, 0, 0,
	 -50, -30, 0, 0, 0, 0, -30, -50, 0, 0, 0, 0, 0, 0, 0, 0,
	 -60, -50, -30, -20, -20, -30, -50, -60, 0, 0, 0, 0, 0, 0, 0, 0,
	 -70, -60, -30, -30, -30, -30, -60, -70, 0, 0, 0, 0, 0, 0, 0, 0}
};

int P_sqpc[2][128] = {
	{
	 1500, 2000, 2000, 2000, 2000, 2000, 1500, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 400, 500, 500, 500, 500, 500, 500, 400, 0, 0, 0, 0, 0, 0, 0, 0,
	 200, 250, 300, 300, 300, 300, 250, 200, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 100, 200, 200, 200, 200, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 -50, 40, 50, 50, 50, 50, 40, -50, 0, 0, 0, 0, 0, 0, 0, 0,
	 -50, 0, 0, 0, 0, 0, 0, -50, 0, 0, 0, 0, 0, 0, 0, 0,
	 -50, 0, 0, 0, 0, 0, 0, -50, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	 - 50, 0, 0, 0, 0, 0, 0, -50, 0, 0, 0, 0, 0, 0, 0, 0,
	 -50, 0, 0, 0, 0, 0, 0, -50, 0, 0, 0, 0, 0, 0, 0, 0,
	 -50, 40, 50, 50, 50, 50, 40, -50, 0, 0, 0, 0, 0, 0, 0, 0,
	 50, 100, 200, 200, 200, 200, 100, 50, 0, 0, 0, 0, 0, 0, 0, 0,
	 200, 250, 300, 300, 300, 300, 250, 200, 0, 0, 0, 0, 0, 0, 0, 0,
	 400, 500, 500, 500, 500, 500, 500, 400, 0, 0, 0, 0, 0, 0, 0, 0,
	 1500, 2000, 2000, 2000, 2000, 2000, 1500, 0, 0, 0, 0, 0, 0, 0, 0}
};

int weak_pawn_pcsq[8] = { -40, -20, -10, -10, -10, -10, -20, -40 };
int passed_pawn_pcsq[2][8] = {
	{2000, 1400, 920, 560, 320, 200, 200, 0},
	{0, 200, 200, 320, 560, 920, 1400, 2000}
};




int check(tree_t *T) {
  return ATTACKS(1 - T->side, T->king[T->side]);
}

void print_move(move_t move)
{
	square_t from = MV_FROM(move);
	square_t to = MV_TO(move);
	printf("%c%c%c%c ", 'a' + SQ_FILE(from), '1' + SQ_RANK(from),
	       'a' + SQ_FILE(to), '1' + SQ_RANK(to));
}

void print_position(tree_t * T)
{
	printf("  +---------------+\n");
	for (int rank = 7; rank >= 0; rank--) {
		printf("%c |", '1' + rank);
		for (int file = 0; file < 8; file++) {
			square_t sq = SQUARE(rank, file);
			if (T->pieces[sq] < 0)
				printf(" ");
			else
				printf("%c",
				       (T->pieces[sq] ? 'p' : 'k') -
				       32 * (T->colors[sq]));
			if (file < 7)
				printf(" ");
		}
		printf("|\n");
	}
	printf("  +---------------+\n");
	printf("   a b c d e f g h  ");
	if (T->side)
		printf("White to play\n");
	else
		printf("Black to play\n");
}

unsigned long long int hash_from_scratch(tree_t * T)
{
	unsigned long long int hash = T->side * zobrist_side;
	for (int sq = 0; sq < 128; sq++)
		if (VALID(sq) && !EMPTY(sq))
			hash ^= zobrist[sq][T->pieces[sq]][T->colors[sq]];
	return hash;
}

int heuristic_evaluation(tree_t * T)
{
	int score =
	    10 * (2 * T->side - 1) + 100 * (T->pawns[WHITE] - T->pawns[BLACK]);
	for (int sq = 0; sq < 128; sq++) {
		if (T->pieces[sq] != PAWN)
			continue;

		int passed = 1;
		int weak = 1;
		int opposed = 0;
		int doubled = 0;
		int mobile = 0;
		int protected_passer = 0;
		int connected_passer = 0;
		int color = T->colors[sq];

		if (ATTACKS(1 - color, sq))
			passed = 0;

		int target = sq + pawn_move[color];
		if (VALID(target) && EMPTY(target))
			mobile = 1;

		while (VALID(target)) {
			if (T->pieces[target] == PAWN) {
				passed = 0;
				if (T->colors[target] == color)
					doubled = 1;
				else
					opposed = 1;
			}
			if (PAWN_ATTACKS(1 - color, target))
				passed = 0;
			target += pawn_move[color];
		}

		target = sq + pawn_move[color];
		while (VALID(target)) {
			if (PAWN_ATTACKS(color, target)) {
				weak = 0;
				break;
			}
			target -= pawn_move[color];
		}
		if (passed && ATTACKS(color, sq))
			protected_passer = 1;
		if (passed && PAWN_ATTACKS(color, sq))
			connected_passer = 1;

		int pawn_value = P_sqpc[color][sq];

		if (mobile)
			pawn_value += 15;
		if (!opposed)
			pawn_value += 3;
		if (passed)
			pawn_value += passed_pawn_pcsq[color][SQ_RANK(sq)];
		if (weak)
			pawn_value += weak_pawn_pcsq[SQ_FILE(sq)];
		if (doubled)
			pawn_value += weak_pawn_pcsq[SQ_FILE(sq)] / 2;

		if (protected_passer)
			pawn_value = (pawn_value * 12) / 10;
		if (connected_passer)
			pawn_value = (pawn_value * 15) / 10;

		score += (2 * color - 1) * pawn_value;
	}

	int k_md_rank = abs(SQ_RANK(T->king[WHITE]) - SQ_RANK(T->king[BLACK]));
	int k_md_file = abs(SQ_FILE(T->king[WHITE]) - SQ_FILE(T->king[BLACK]));
	int opposition = ((k_md_rank % 2) == 0 && (k_md_file % 2) == 0);
	if (opposition) {
		score += (2 * T->side - 1) * 50;
	}

        if (score == 0)
          score += 2*T->side - 1;
	return score;
}


void compute_attack_squares(tree_t *T) {
  	for (int sq = 0; sq < 128; sq++)
		T->attack[sq] = 0;

	for (int sq = 0; sq < 128; sq++) {
		if (T->pieces[sq] < 0)
			continue;
		int piece = T->pieces[sq];
		int color = T->colors[sq];

		for (int i = 0; i < n_attack_vectors[piece]; i++) {
			int target = sq + attack_vectors[piece][color][i];
			if (VALID(target)) {
				T->attack[target] |=
				    (1 + piece) * (1 + 15 * color);
			}
		}
	}
}


int generate_legal_moves(tree_t *T, move_t *moves) {
  int n_moves = 0;
          for (int sq = 0; sq < 128; sq++) {
		int color = T->colors[sq];
		if (color == T->side) {
			if (T->pieces[sq] == PAWN) {

				int target = sq + pawn_move[color];
				if (VALID(target) && EMPTY(target)) {
					ENQUEUE_MOVE(target);

					target += pawn_move[color];
					if (VALID(target)) {
						if (T->side == WHITE
						    && SQ_RANK(sq) == 1
						    && EMPTY(target))
							ENQUEUE_MOVE(target);
						if (T->side == BLACK
						    && SQ_RANK(sq) == 6
						    && EMPTY(target))
							ENQUEUE_MOVE(target);
					}
				}

				for (int i = 0; i < 2; i++) {
					int target =
					    sq + attack_vectors[PAWN][color][i];
					if (VALID(target) && ENEMY(target))
						ENQUEUE_MOVE(target);
				}
			} else {
				for (int i = 0; i < 8; i++) {
					int target =
					    sq + attack_vectors[KING][color][i];
					if (VALID(target) && !FRIENDLY(target)
					    && !(ATTACKS(1 - T->side, target)))
						ENQUEUE_MOVE(target);
				}
			}
		}
	}
          return n_moves;
}



#define SWAP_MOVE {int tmp= moves[k];moves[k++]= moves[i];moves[i]= tmp;}

void sort_moves(tree_t *T, int n_moves, move_t *moves) {
        int k = 0;
        if (T->suggested_move != BAD_MOVE)
          for(int i = k; i < n_moves; i++)
            if (T->suggested_move == moves[i])
              SWAP_MOVE;

	for (int i = k; i < n_moves; i++)
		if (CAPTURE(moves[i]))
			SWAP_MOVE;
	for (int i = k; i < n_moves; i++)
		if (PAWN_MOVE(moves[i]))
			SWAP_MOVE;
}


void play_move(tree_t *T, move_t move, tree_t *child) {
  child->depth = T->depth - 1;
  child->height = T->height + 1;
  child->alpha_start = child->alpha = -T->beta;
  child->beta = -T->alpha;
  child->suggested_move = BAD_MOVE;
  
  child->side = 1 - T->side;
  for (int sq = 0; sq < 128; sq++) {
    child->pieces[sq] = T->pieces[sq];
    child->colors[sq] = T->colors[sq];
  }
  
  square_t from = MV_FROM(move);
  square_t to = MV_TO(move);
  
  child->pieces[to] = child->pieces[from];
  child->colors[to] = child->colors[from];
  child->colors[from] = child->pieces[from] = -1;
  
  child->pawns[WHITE] = T->pawns[WHITE];
  child->pawns[BLACK] = T->pawns[BLACK];
  child->king[WHITE] = T->king[WHITE];
  child->king[BLACK] = T->king[BLACK];
  
  if (T->pieces[to] == PAWN)
    child->pawns[T->colors[to]]--;
  if (T->pieces[from] == KING)
    child->king[T->colors[from]] = to;
    
  child->hash = T->hash ^ zobrist_side;
  child->hash ^= zobrist[from][T->pieces[from]][T->colors[from]];
  child->hash ^= zobrist[to][T->pieces[from]][T->colors[from]];
  if (T->pieces[to] >= 0)
    child->hash ^= zobrist[to][T->pieces[to]][T->colors[to]];
  
  for(int i = 0; i < T->height; i++)
    child->history[i] = T->history[i];
  child->history[T->height] = T->hash;
}


void parse_FEN(const char *FEN, tree_t *root) {
	for (int sq = 0; sq < 128; sq++)
		root->colors[sq] = root->pieces[sq] = -1;
	root->pawns[WHITE] = root->pawns[BLACK] = 0;
	root->king[WHITE] = root->king[BLACK] = -1;

	int rank = 7;
	int file = 0;
	int ptr = 0;
	do {
		int sq = SQUARE(rank, file);
		switch (FEN[ptr++]) {
		case 'K':
			root->colors[sq] = WHITE;
			root->pieces[sq] = KING;
			root->king[WHITE] = sq;
			file++;
			break;
		case 'k':
			root->colors[sq] = BLACK;
			root->pieces[sq] = KING;
			root->king[BLACK] = sq;
			file++;
			break;
		case 'P':
			root->colors[sq] = WHITE;
			root->pieces[sq] = PAWN;
			root->pawns[WHITE]++;
			file++;
			break;
		case 'p':
			root->colors[sq] = BLACK;
			root->pieces[sq] = PAWN;
			root->pawns[BLACK]++;
			file++;
			break;
		case '/':
			rank--;
			file = 0;
			break;
		case '1':
			file += 1;
			break;
		case '2':
			file += 2;
			break;
		case '3':
			file += 3;
			break;
		case '4':
			file += 4;
			break;
		case '5':
			file += 5;
			break;
		case '6':
			file += 6;
			break;
		case '7':
			file += 7;
			break;
		case '8':
			file += 8;
			break;
		};
	}
	while (FEN[ptr] != ' ');

	root->side = (FEN[++ptr] == 'w') ? WHITE : BLACK;
	if (root->king[WHITE] < 0 || root->king[BLACK] < 0)
		errx(2, "les rois doivent être présents");
	root->hash = hash_from_scratch(root);
        root->suggested_move = BAD_MOVE;
}


int test_draw_or_victory(tree_t *T, result_t *result) {
        /* teste les nulles faciles */
	if (T->pawns[WHITE] + T->pawns[BLACK] == 0) {
          result->score = CERTAIN_DRAW;
          return 1;
        }

        for(int i = 0; i < T->height; i++)
          if (T->hash == T->history[i]) {
            result->score = CERTAIN_DRAW;
            return 1;
          }
        
        /* teste si j'ai gagné */
	int last_rank = SQUARE(7 * T->side, 0);
	for (int i = 0; i < 8; i++) {
		if (T->pieces[last_rank] == PAWN && T->colors[last_rank] == T->side)  {
                  result->score = MAX_SCORE;
                  return 1;
                }
		last_rank += EAST;
	}
        return 0;
}


void init_tt() {
  tt_probe = tt_hit = tt_partial = 0;
  tt = malloc(TT_SIZE * sizeof(tt_entry_t));
  if (tt == NULL)
      err(1, "impossible d'allouer tt");
  for (int i = 0; i < TT_SIZE; i++)
    tt[i].flag = TT_INVALID;
}


void free_tt() {
  printf("TT hit ratio : %.1f %% (partial %.1f %%)\n", (100.0 * tt_hit) / tt_probe, (100.0 * tt_partial) / tt_probe);
  int fill = 0;
  for (int i = 0; i < TT_SIZE; i++)
    if (tt[i].flag != TT_INVALID)
      fill++;
  printf("TT fill ratio : %.1f %%\n", (100.0 * fill) / TT_SIZE);
  
  free(tt);
}


int tt_fetch(tree_t *T, result_t *result) {
  unsigned long long int idx = (T->hash % TT_SIZE);

  if (tt[idx].flag == TT_INVALID)
    return 0;

  if (tt[idx].hash != T->hash)
      return 0;

  if (tt[idx].flag != TT_EXACT)
    return 0;

  result->score = tt[idx].score;
  result->best_move = tt[idx].best_move;
  result->pv_length = 0;

  return 1;
}
  

int tt_lookup(tree_t *T, result_t *result)
{
  unsigned long long int idx = (T->hash % TT_SIZE);
  tt_probe++;
  
  if (tt[idx].flag == TT_INVALID)
    return 0;

  if (tt[idx].hash != T->hash)
      return 0;

  tt_partial++;
  T->suggested_move = tt[idx].best_move;

  if (tt[idx].depth < T->depth)
    return 0;
    
  tt_hit++;
  
  result->score = tt[idx].score;
  result->best_move = tt[idx].best_move;
  result->pv_length = 1;
  result->PV[0] = result->best_move;
  
  if (tt[idx].flag == TT_EXACT)
    return 1;
  
  if (tt[idx].flag == TT_LOWER_BOUND)
    T->alpha = MAX(T->alpha, tt[idx].score);
  else
    T->beta = MIN(T->beta, tt[idx].score);
  
  if (T->alpha >= T->beta)
    return 1;

  return 0;
}

void tt_store(tree_t *T, result_t *result) {
  unsigned long long int idx = (T->hash % TT_SIZE);
  
  int flag = TT_EXACT;
  if (result->score <= T->alpha_start) {
    flag = TT_UPPER_BOUND;
  } else if (result->score >= T->beta) {
    flag = TT_LOWER_BOUND;
  }

  tt[idx].hash = T->hash;
  tt[idx].depth = T->depth;
  tt[idx].flag = flag;
  
  tt[idx].score = result->score;
  tt[idx].best_move = result->best_move;
}
              

void print_pv(tree_t *T, result_t *result) {
  tree_t *a, *b, *tmp;
  tree_t ta, tb;
  result_t r;
  play_move(T, result->best_move, &ta);
  a = &ta;
  b = &tb;
  print_move(result->best_move);
  
#if 1
  for(int i = 1; i < result->pv_length; i++) {
    move_t move = result->PV[i];
    play_move(a, move, b);
    int stop = 0;
    for(int i = 0; i < a->height; i++)
      if (b->hash == a->history[i])
        stop = 1;
    if (stop) break;
    
    print_move(move);
    tmp = a;
    a = b;
    b = tmp;
  }
  if (!test_draw_or_victory(a, &r))
    printf("...");
  printf("\n");
  print_position(a);
#else
   /* tente de reconstituer la variation principale à partir de la table de hachage */
  while(tt_fetch(a, &r)) {
    move_t move = r.best_move;
    play_move(a, move, b);
    int stop = 0;
    for(int i = 0; i < a->height; i++)
      if (b->hash == a->history[i])
        stop = 1;
    if (stop) break;
    
    print_move(move);
    tmp = a;
    a = b;
    b = tmp;
  }
  if (!test_draw_or_victory(a, &r))
    printf("...");
  printf("\n");
  print_position(a); 
#endif
}
