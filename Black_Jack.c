/*
 * Author: Noga Avraham
 * Description: .cpp Implementation of the "Black Jack" game (using generelized void* data storing Single Linked List).
                [Real-Time Group C course project]
 * Language:  C
 * Date: July 2021
*/

#include<stdio.h>
#include<inttypes.h>
#include<string.h>
#include <ctype.h>
#include<stdlib.h>
#include<time.h>//srand()
#include<math.h>
#include"SLL.h" //for SLL's: Deck, player_hand and dealer_hand
#include "Black_Jack.h"


#define MIN_CASH 1000
#define HOUSE_CASH_LIMIT 1000 //multiplier of MIN_CASH for house max cash budget (Note for tester: I added this limitation)
#define DECK_SIZE 52
#define CARDS_IN_SET 13
#define BLACK_JACK 21
#define MAX_NAME_LEN 15
#define ATTEMPTS 3


enum check_card_states{ RESET_CARDS=1, LOOSE_BET, CONTINUE_BET};
enum game_states{STOP_GAME, CONTINEU_GAME, CONTINEU_HIT};

//GLOBALS
static const char* suits[] = { "SPADES", "HEARTS", "CLUBS", "DIAMONDS" };
static const char* cards_symbols[CARDS_IN_SET] = { "Ace", "2", "3", "4", "5", "6", "7", "8", "9", "10", "Jack", "Queen", "King" };

//suit_rank[] values are pointed to by SLL nodes (by void *data). This array is built once at game_init() and doesn't change after.
static uint8_t suit_rank[DECK_SIZE] = { 0 };// [1:0] bits kept for card suit.  [5:2] bits kept for card rank. [6-7] empty
static const char currency = '$';
static unsigned int moves_counter = 0;

//STRUCTS
typedef struct Person{
	char _name[MAX_NAME_LEN];
	int32_t _id;
}Person_t;

typedef struct Account {
	int32_t _cash;
	uint32_t _bet;
}Account_t;

typedef struct Player {
	Person_t _info; //'Dealer' _info.id = 0
	Account_t _account;
	List* _cards;
}Player_t;

//This struct is used as args to sum() pointer function 
typedef struct sum_args {
	uint32_t sum;
	uint8_t aces;
}sum_args_t;


//STATIC PROTOTYPES - to be used internaly only by this .cpp file
//-----------------
//initialization functions:
static int play();
static void game_init(List** dealer_hand, List** player_hand, List** deck);
static int cach_deposit_request(Player* player);
static void build_deck(List* deck);

//print functions:
static const char* extract_suit(uint8_t *card);
static const char* extract_rank(uint8_t* card);
static void display_cards(Player* player, size_t start_pos, size_t end_pos);
void print_card(void* card);
static void print_winner_loser(Player_t* loser, Player_t* winner);

//game phases functions:
static int bet(Player_t* player, Player_t* dealer);
static int deal(Player_t* dealer, Player_t* player, List* deck);
static int hit_or_stand(Player_t* dealer, Player_t* player, List* deck);
static int random_draw(List* deck, Player_t* player, size_t count, bool display);
static bool reset_cards(Player_t* dealer, Player_t* player, List* deck);

//handler functions:
static void win_lose_transactions(Player_t* loser, Player_t* winner, double transact_multiplier);
static uint32_t calculate_hand_val(List* cards);
static int player_cards_check(Player_t* player, Player_t* dealer);
static void clear_input(void);

//free resources
static void clearAll(Player_t* player, Player_t* dealer, List* deck);
//error prints and exit
static void assert_condition(bool isValid, const char* errorMsg, bool isFatal);



//This is the only function exposed to player/main in header.
//Returns: FAIL (-1 int), otherwise returns SUCCESS (0).
int play() {

	Player_t dealer = { {"Dealer", 0}, {MIN_CASH * HOUSE_CASH_LIMIT, 0}, NULL };
	Player_t player = { 0 };
	List* deck = NULL;
	bool new_round = true;
	int hit_stand = { 0 };
	uint8_t attempts = ATTEMPTS;

	printf("\nWellcome to the 'Black-Jack' betting game!\n"
		"******************************************\n"
		"Enter your name : ");

	//player can put any name or nick-name he chooses (all chars are valid)
	fgets(player._info._name, MAX_NAME_LEN, stdin);
	player._info._name[strcspn(player._info._name, "\n")] = '\0';

	//id 0 reserved to the dealer. id cannot be negative
	while (player._info._id <= 0 && attempts) {
		printf("%s, Enter your ID : ", player._info._name);
		scanf("%u", &player._info._id);
		--attempts;

		//Invalid input of alphabet chars can also result in invalid 0
		if (player._info._id == 0) {
			printf("ID must contain digits only, and not 0. Try again\n");
		}
		clear_input();
	}
	if (!attempts) {
		printf("%s, Your %d attempts to input valid ID failed. Please see Cazino manager.\n", player._info._name, ATTEMPTS);
		return FAIL;
	}

	if (cach_deposit_request(&player) == STOP_GAME)
		return FAIL;

	game_init(&dealer._cards, &player._cards, &deck);

	while (new_round) {

		if (bet(&player, &dealer) != SUCCESS) //Betting phase 
			break;

		if (deal(&dealer, &player, deck) != SUCCESS) //Initial Deal phase 
			break;

		switch (player_cards_check(&player, &dealer)) { //Black Jack phase

		case RESET_CARDS:
			new_round = reset_cards(&dealer, &player, deck);
			break;
		case LOOSE_BET:
			win_lose_transactions(&player, &dealer, 1);
			new_round = false;
			break;
		case CONTINUE_BET:
			do {
				hit_stand = hit_or_stand(&dealer, &player, deck);  //Hit or Stand phase
			} while (hit_stand == CONTINEU_HIT);

			new_round = hit_stand;//hit_stand 1(true) or 0(false)
			break;
		}
		moves_counter = 0;

	}

	//free resources
	clearAll(&player, &dealer, deck);
	printf("\nGAME-OVER\n");

	return SUCCESS;
}

//This function is sent to SSL 'print_list()' as pointer to printing function.
void print_card(void *card) {
	const char* suit = extract_suit((uint8_t*)card);
	const char* rank = extract_rank((uint8_t*)card);
	printf(" [%s of %s] ", rank, suit);
}

static void display_cards(Player* player, size_t start_pos, size_t end_pos){
	assert_condition(player, "Error: function[display_cards()]: pointer provided to argument 'player' is Null. exitting", true);

	   printf("   %-*s", MAX_NAME_LEN, player->_info._name);
       print_list_by_range(player->_cards, start_pos, end_pos, print_card);
}

static const char* extract_suit(uint8_t *card) {
	assert_condition(card, "Error: function[extract_suit()]: pointer provided to argument 'card' is Null. exitting", true);

	return suits[(*card) & 0x03];
}
static const char* extract_rank(uint8_t *card) {
	return cards_symbols[((*card) >> 2)];
}


static void build_deck(List* deck) {
	assert_condition(deck, "Error: function[build_deck()]: pointer to 'deck' list is Null. exitting", true);

	uint8_t cards_sets = DECK_SIZE / CARDS_IN_SET; //default: 4
	uint8_t index = 0;

	for (uint8_t i = 0; i < cards_sets; ++i) {
		for (uint8_t j = 0; j < CARDS_IN_SET; ++j) {
			index = i * CARDS_IN_SET + j;
			suit_rank[index] |= i;
			suit_rank[index] |= (j<<2);

			add_to_back(deck, create_node((void*)&suit_rank[index]));
		}
	}
}

static void game_init(List** dealer_hand, List** player_hand, List** deck) {
	assert_condition(dealer_hand, "Error: function[game_init()]: pointer provided to argument 'dealer_hand**' is Null. exitting", true);
	assert_condition(player_hand, "Error: function[game_init()]: pointer provided to argument 'player_hand**' is Null. exitting", true);
	assert_condition(deck, "Error: function[game_init()]: pointer provided to argument 'deck**' is Null. exitting", true);

	srand((unsigned int)time(NULL));//used in random_draw()

	*deck = create_list();
	build_deck(*deck);

	*dealer_hand = create_list();
	*player_hand = create_list();
}

static void print_winner_loser(Player_t* loser, Player_t* winner) {
	assert_condition(loser, "Error: function[print_winner_loser()]: pointer provided to argument 'loser' is Null. exitting", true);
	assert_condition(winner, "Error: function[print_winner_loser()]: pointer provided to argument 'winner' is Null. exitting", true);

	uint32_t winner_result = calculate_hand_val(winner->_cards);
	printf("\n     $ $ $ $ $ $ $ $\n"
		   "#%u) |WIN LOSE status|:\n"
		   "     $ $ $ $ $ $ $ $\n"
	       "-------------------------------------\n", ++moves_counter);

	printf("%s WINS with cards: ", winner->_info._name);
	print_list(winner->_cards, print_card);
	if(winner_result == 21)
		printf("Cards value: 21 BLACK JACK !!!\n");
	else {
		printf("Cards value: %u\n", winner_result);
	}
	printf("\n%s LOSES with cards: ", loser->_info._name);
	print_list(loser->_cards, print_card);
	printf("Cards value: %u\n", calculate_hand_val(loser->_cards));
}

//Transacting money from the loser's account to the winner's account, 
//Transaction amount calculated as: transact_multiplier*winner->_account._bet
static void win_lose_transactions(Player_t* loser, Player_t* winner, double transact_multiplier) {
	assert_condition(loser, "Error: function[win_lose_transactions()]: pointer provided to argument 'loser' is Null. exitting", true);
	assert_condition(winner, "Error: function[win_lose_transactions()]: pointer provided to argument 'winner' is Null. exitting", true);
	assert_condition(transact_multiplier > 0, "Warning: function[win_lose_transactions()]: 'transact_multiplier' should not be 0 or negative", false);

	print_winner_loser(loser, winner);
	uint32_t money_to_transact = { 0 };

	//Dealer loses, Player wins
	if (!loser->_info._id) { //Dealer id is always 0

		money_to_transact = (uint32_t)(transact_multiplier * winner->_account._bet);
		printf("\n%s BUST!\n", loser->_info._name);

		if (loser->_account._cash <= (int32_t)money_to_transact) {
			printf("The house budget for this game ran out. ");

			if (loser->_account._cash < (int32_t)money_to_transact)
				printf("and the house uses extra cash budget of %d%c to fully pay the winner %s.\n",
					    money_to_transact - loser->_account._cash, currency, winner->_info._name);
		}
		loser->_account._cash -= money_to_transact; //House/Dealer cash may get negative here if is in debt to player
		winner->_account._cash += money_to_transact;
		winner->_account._cash += winner->_account._bet; //bet returns to player
		winner->_account._bet = 0;
		printf("%s, You WIN! your account is rewarded with %.1lf times your bet (i.e: %u%c)."
			    "  [Your current cash: %d%c. Current bet: %u%c]\n", 
			     winner->_info._name, transact_multiplier, money_to_transact, currency, winner->_account._cash, currency, winner->_account._bet, currency);
	}
	else { //Player loses, Dealer wins

		money_to_transact = (uint32_t)(transact_multiplier * loser->_account._bet);
		winner->_account._cash += money_to_transact; //house gets player's money
		loser->_account._bet = 0;
		printf("\nBUST! %s, You lose! %.1lf times your bet was subtracted from your account. [Your current cash: %d%c. Current bet: %u%c].\n",
			loser->_info._name, transact_multiplier, loser->_account._cash, currency, loser->_account._bet, currency);
		printf("%s WINS!", winner->_info._name);

	}
}

static bool dealer_draw(Player_t* dealer, Player_t* player, List* deck) {
	assert_condition(dealer, "Error: function[dealer_draw()]: pointer provided to argument 'dealer' is Null. exitting", true);
	assert_condition(player, "Error: function[dealer_draw()]: pointer provided to argument 'player' is Null. exitting", true);
	assert_condition(deck, "Error: function[dealer_draw()]: pointer provided to argument 'deck' is Null. exitting", true);

	printf("\n#%u)       DEALER DRAWS:\n"
		   "-------------------------------------\n", ++moves_counter);
	uint32_t player_hand_val = calculate_hand_val(player->_cards);
	uint32_t dealer_hand_val = 0;

	if (calculate_hand_val(dealer->_cards) > player_hand_val) {
		win_lose_transactions(player, dealer, 1);
		return reset_cards(dealer, player, deck); //returns: STOP_GAME/CONTINUE_GAME
	}

	while ((dealer_hand_val = calculate_hand_val(dealer->_cards)) <= player_hand_val && dealer_hand_val < 17) {
		random_draw(deck, dealer, 1, true);
	}


	if (dealer_hand_val > BLACK_JACK) {
		win_lose_transactions(dealer, player, 2);
	}
	else if (dealer_hand_val == BLACK_JACK) {
		win_lose_transactions(player, dealer, 1);
	}
	else{

		if (dealer_hand_val == player_hand_val) {
			printf("TIE!\n");
		}
		else if (dealer_hand_val < player_hand_val) {
			win_lose_transactions(dealer, player, 1);
		}
		else {
			win_lose_transactions(player, dealer, 1);
		}
	}
	return reset_cards(dealer, player, deck);//returns: STOP_GAME(0)/CONTINUE_GAME(1)
}

//returns: STOP_GAME(0)/CONTINUE_GAME(1)
static int hit_or_stand(Player_t* dealer, Player_t *player, List* deck) {
	assert_condition(dealer, "Error: function[hit_or_stand()]: pointer provided to argument 'dealer' is Null. exitting", true);
	assert_condition(player, "Error: function[hit_or_stand()]: pointer provided to argument 'player' is Null. exitting", true);
	assert_condition(deck, "Error: function[hit_or_stand()]: pointer provided to argument 'deck' is Null. exitting", true);

	uint32_t hand_value = 0;

	printf("\n#%u)      HIT OR STAND:\n"
	       "-------------------------------------\n", ++moves_counter);

	char hit_stand = '?';

	while (hit_stand != 'H' && hit_stand != 'S') {
		getchar();
		printf("Enter 'H' (to hit) or 'S' (to stand): ");
		scanf("%c", &hit_stand);
		if (isalpha(hit_stand)) { hit_stand = toupper(hit_stand); }
	}
	if (hit_stand == 'S') {
		printf("\n\nSTAND!\n");
		//revealing the dealer's cards
		printf("Dealer cards revealed:    ");
		print_list(dealer->_cards, print_card);
		return dealer_draw(dealer, player, deck);//returns: STOP_GAME(0)/CONTINUE_GAME(1)
	}
	else {
		printf("\n\nHIT!\n");
		random_draw(deck, player, 1, true);//drawing single card to player
		printf("%s, your list of cards after draw: ", player->_info._name);
		print_list(player->_cards, print_card);
		hand_value = calculate_hand_val(player->_cards);
		printf("\nYour hand value after draw is: %u\n\n",hand_value);
		if (hand_value > BLACK_JACK) {
			win_lose_transactions(player, dealer, 1);
			return reset_cards(dealer, player, deck);
		}
		if (hand_value == BLACK_JACK) {
			win_lose_transactions(dealer, player, 1);
			return reset_cards(dealer, player, deck);
		}
		return CONTINEU_HIT;
	}
}



static void clearAll(Player_t* player, Player_t *dealer, List *deck) {
	clear_list(player->_cards);
	free(player->_cards);

	clear_list(dealer->_cards);
	free(dealer->_cards);

	clear_list(deck);
}

//Adds all the cards in the players and dealers hand to the top of the deck. 
//If the player's cash is less than 10, the game is over.
//returns true- continue to play, or false- stop game.
static bool reset_cards(Player_t *dealer, Player_t *player, List* deck) {
	assert_condition(dealer, "Error: function[reset_cards()]: pointer provided to argument 'dealer' is Null. exitting", true);
	assert_condition(player, "Error: function[reset_cards()]: pointer provided to argument 'player' is Null. exitting", true);
	assert_condition(deck, "Error: function[reset_cards()]: pointer provided to argument 'deck' is Null. exitting", true);

	printf("\n\n#%u)     CARDS RESETTING:\n"
		   "-------------------------------------\n", ++moves_counter);
	char continu = 0;
	Node_t* node;

	while (node = pop(dealer->_cards)) {
		push(deck, node);
	}

	while (node = pop(player->_cards)) {
		push(deck, node);
	}

	if (player->_account._cash < 10 || dealer->_account._cash < 10) {
		if(player->_account._cash < 10) printf("Sorry %s, You are out of cash to bet  :(\n", player->_info._name);
		if (dealer->_account._cash < 10) printf("House budget for this game ran out.\n");
		return STOP_GAME;
	}

	while (continu != 'Y' && continu != 'N') {
		getchar();
		printf("%s, Would you like to bet again? [Y/N]\n", player->_info._name);
		scanf("%c", &continu);
		if (isalpha(continu)) { continu = toupper(continu); }
	}
	return (continu == 'Y' ? CONTINEU_GAME : STOP_GAME);
}

static int cach_deposit_request(Player *player) {
	assert_condition(player, "Error: function[cach_deposit_request()]: pointer provided to argument 'player' is Null. exitting", true);

	uint32_t cash = 0;
	uint8_t attempts = ATTEMPTS;

	printf("\n#%u)     CASH DEPOSITING:\n"
		   "-------------------------------------\n"
		   "%s, How much CASH would you like to deposite?\n"
		   "Your current cash: %d%c.   (NOTE: Minimum deposit amount: 1,000$, in multiples of 10)\n"
		    , ++moves_counter, player->_info._name, player->_account._cash, currency);

	scanf("%d", &cash);
	//check valid input amount(cash must be at least 1,000, in 10's) 
	while (attempts && ((player->_account._cash + cash < MIN_CASH) || cash % 10 != 0)) {
		printf("Invalid input. No deposit occured. Try again:\n");
		scanf("%d", &cash);
		--attempts;
	}

	if (!attempts) {
		printf("%s, Your %d attempts to deposit cash have failed. Please see Cazino manager\n", player->_info._name, ATTEMPTS);
		return STOP_GAME;
	}

	player->_account._cash = cash;
	return CONTINEU_GAME;
}

static int bet_request(Player* player) {
	assert_condition(player, "Error: function[bet_request()]: pointer provided to argument 'player' is Null. exitting", true);

	uint8_t attempts = ATTEMPTS;
	uint32_t bet = 0;

	printf("%s, How much to add to your BET?\n"
		    "Your current bet is: %u%c   [Your current cash: %d%c]. (Add in multiples of 10 only.)\n"
		     , player->_info._name, player->_account._bet, currency, player->_account._cash, currency);

	scanf("%u", &bet);
	//check valid input amount(bet must be added in multiples of 10. player can add 0 only if bet>0) 
	while (attempts && ((player->_account._bet + bet > (uint32_t)player->_account._cash)||
		               !(player->_account._bet + bet) ||
		                 bet % 10)) {
		printf("Invalid input. No bet adding occured.\n");

		if (--attempts) {
			printf("Try again: ");
			scanf("%u", &bet);
		}
		else {
			return FAIL; //returning to while loop with option for user to end the game
		}
	}

	player->_account._bet += bet;
	player->_account._cash -= bet;

	return SUCCESS;
}

//draws 'count' number of cards from 'deck' and insert to the end of player's deck 
static int random_draw(List* deck, Player_t* player, size_t count, bool display) {
	assert_condition(deck, "Error: function[random_draw()]: pointer to 'deck' list is Null. exitting", true);
	assert_condition(player, "Error: function[random_draw()]: pointer to 'player' is Null. exitting", true);
	
	size_t pos = -1;

	if (count > deck->_count) {
		printf("Cannot draw more cards than exist in deck. Number of cards in deck is: %zu", deck->_count);
		return FAIL;
	}

	for (size_t i = 0; i < count; ++i) {
		pos = rand() % deck->_count + 1;//draws random positions in range 1-52
		Node_t* card = remove_at(deck, pos);
		add_to_back(player->_cards, card);

		if (display) { 
			printf("Added card to %s:  ", player->_info._name);
			print_card(card->_data);
			puts("");
		}
	}

	return SUCCESS;
}

//used as pointer to function to be sent for for_each() SLL 
//returnes number of Ace's in the cards
void sum(void* data, void* args) {
	assert_condition(data, "Error: function[sum()]: pointer to 'data' is Null. exitting", true);
	assert_condition(args, "Error: function[sum()]: pointer to 'args' is Null. exitting", true);

	uint8_t rank = (*(uint8_t*)data >> 2) +1; //1 is added because ranks range is 0-12
	if (rank == 1) {
		((sum_args_t*)args)->aces++;
	}

	if (rank >10) {  //Jack, Queen and King value is 10
		((sum_args_t*)args)->sum += 10;
	}
	else {
		((sum_args_t*)args)->sum += rank;
	}
}

static uint32_t calculate_hand_val(List* cards) {
	assert_condition(cards, "Error: function[calculate_hand_val()]: pointer provided to argument 'cards' is Null. exitting", true);

	sum_args_t args = { 0 };

	for_each(cards, (void*)&args, sum); 

	while (args.aces > 0 && (args.sum + 10 <= 21)) {
		args.sum += 10;
		args.aces--;
	}
	return args.sum;

}


static int player_cards_check(Player_t *player, Player_t* dealer) {
	assert_condition(player, "Error: function[player_cards_check()]: pointer provided to argument 'player' is Null. exitting", true);
	assert_condition(dealer, "Error: function[player_cards_check()]: pointer provided to argument 'dealer' is Null. exitting", true);

	uint32_t cards_value = calculate_hand_val(player->_cards);
	if (cards_value == BLACK_JACK) {
		printf("BLACK-JACK !!!\n");
		win_lose_transactions(dealer, player, 1.5);
		return RESET_CARDS;
	}
	else if (cards_value > BLACK_JACK) {
		return LOOSE_BET;
	}
	else {
		return CONTINUE_BET;
	}
}

static int deal(Player_t* dealer, Player_t* player, List* deck) {
	assert_condition(deck, "Error: function[deal()]: pointer to 'deck' list is Null. exitting", true);
	assert_condition(dealer, "Error: function[deal()]: pointer to 'dealer' is Null. exitting", true);
	assert_condition(player, "Error: function[deal()]: pointer to 'player' is Null. exitting", true);

	printf("\n#%u)        DEALLING:\n"
		   "-------------------------------------\n", ++moves_counter);
	if ((random_draw(deck, dealer, 2, false) | random_draw(deck, player, 2, false)) != 0) {
		printf("Failed to deal cards to players.\n");
		return FAIL;
	}

	printf("Cards delt:\n");
	//dealer reveals only the one card before last in his card list (cards added to back of list in draw)
	display_cards(dealer, dealer->_cards->_count - 1, dealer->_cards->_count - 1);
	printf("   ????????\n");
	//player reveals two last cards in his card list
	display_cards(player, player->_cards->_count-1, player->_cards->_count);
	puts("\n\n");

	return SUCCESS;
}

static int bet(Player_t *player, Player_t *dealer) {
	assert_condition(player, "Error: function[bet()]: pointer provided to argument 'player' is Null. exitting", true);
	assert_condition(dealer, "Error: function[bet()]: pointer provided to argument 'dealer' is Null. exitting", true);

	printf("\n#%u)          BETTING:\n"
	        "-------------------------------------\n", ++moves_counter);
	char continu='?';

		if (player->_account._cash == 0) {
			while (continu != 'Y' && continu != 'N') {
				printf("%s, you have no cash left on your account.\n"
				    	"Would you like to deposit to cash and continue betting? [Y/N]\n", player->_info._name);
				scanf("%c", &continu);

				if (isalpha(continu)) { toupper(continu); }
			}
			if (continu == 'N') return STOP_GAME;

			if (cach_deposit_request(player) == STOP_GAME)
				return STOP_GAME;
		}

		if (player->_account._cash < 0) {//this state should never occure.
			return FAIL;
		}
			
		//dded a dealer/house budget limit of cash for this game
		if (dealer->_account._cash <= 0) {
			return STOP_GAME;
		}

		if(bet_request(player) == FAIL) {
			printf("%s, you failed to add bet. Game ends.\n", player->_info._name);
			return FAIL;
		}
		return SUCCESS;
}

void assert_condition(bool isValid, const char* errorMsg, bool isFatal) {

	if (!errorMsg) {
		fprintf(stderr, "%s","Error: function[assert_condition()]: pointer provided to argument 'errorMsg' is Null. exitting" );
		exit(EXIT_FAILURE);
	}

	if (!isValid) {
		if (errorMsg) {
			fprintf(stderr, "%s", errorMsg);
		}
		if (isFatal) {
			exit(EXIT_FAILURE);
		}
	}
}

static void clear_input(void) {	
	while (getchar() != '\n');
}


int main() {
	play();
	return 0;
}





