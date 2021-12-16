#include <eid-viewer/eid-viewer.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <jansson.h>
#include <sqlite3.h>

#define GREEN(s) "\033[32;1m" s "\033[0m"
#define BLUE(s) "\033[34;1m" s "\033[0m"

#define DBFILE  "/tmp/idcards.sqlite3"

typedef struct card_t {
    char *niss;
    char *cardid;
    json_t *fields;
    uint8_t *photo;
    int photolen;

} card_t;

card_t global_card = {
    .niss = NULL,
    .cardid = NULL,
    .fields = NULL,
    .photo = NULL,
    .photolen = 0,
};

int cardcommit() {
    card_t *card = &global_card;

    sqlite3 *db;
    sqlite3_stmt *stmt;

    printf("[+] card: read complete, saving into database\n");
    char *jsondump = json_dumps(card->fields, 0);

    if(sqlite3_open(DBFILE, &db) != SQLITE_OK) {
        fprintf(stderr, "[-] database: open failed: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    char *query = "INSERT INTO cards (niss, cardid, fields, photo) VALUES (?, ?, ?, ?)";
    if(sqlite3_prepare_v2(db, query, -1, &stmt, 0) != SQLITE_OK) {
        fprintf(stderr, "[-] database: prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    if(sqlite3_bind_text(stmt, 1, card->niss, strlen(card->niss), NULL) != SQLITE_OK)
        fprintf(stderr, "[-] database: bind niss: %s\n", sqlite3_errmsg(db));

    if(sqlite3_bind_text(stmt, 2, card->cardid, strlen(card->cardid), NULL) != SQLITE_OK)
        fprintf(stderr, "[-] database: bind cardid: %s\n", sqlite3_errmsg(db));

    if(sqlite3_bind_text(stmt, 3, jsondump, strlen(jsondump), NULL) != SQLITE_OK)
        fprintf(stderr, "[-] database: bind fields: %s\n", sqlite3_errmsg(db));

    if(sqlite3_bind_blob(stmt, 4, card->photo, card->photolen, NULL) != SQLITE_OK)
        fprintf(stderr, "[-] database: bind photo: %s\n", sqlite3_errmsg(db));

    if(sqlite3_step(stmt) != SQLITE_DONE)
        fprintf(stderr, "[-] database: execute statement: %s\n", sqlite3_errmsg(db));

    if(sqlite3_finalize(stmt) != SQLITE_OK)
        fprintf(stderr, "[-] database: finalize statement: %s\n", sqlite3_errmsg(db));

    sqlite3_close(db);
    free(jsondump);

    printf(GREEN("[+] card: database updated\n"));

    return 0;
}

void newstringdata(const EID_CHAR *label, const EID_CHAR *data) {
    card_t *card = &global_card;

    // printf("[+] label: string data <%s> [%s]\n", label, data);

    json_object_set_new(card->fields, label, json_string(data));

    if(strcmp(label, "national_number") == 0) {
        printf("[+] card: fetching niss: %s\n", data);
        card->niss = strdup(data);
    }

    if(strcmp(label, "card_number") == 0) {
        printf("[+] card: fetching card number: %s\n", data);
        card->cardid = strdup(data);
    }
}

void newbindata(const EID_CHAR *label, const unsigned char *data, int datalen) {
    card_t *card = &global_card;

    // printf("[+] label: binary data <%s> [%d bytes]\n", label, datalen);

    if(strcmp(label, "PHOTO_FILE") == 0) {
        printf("[+] card: fetching photo: %d bytes\n", datalen);
        card->photo = malloc(datalen);
        card->photolen = datalen;

        memcpy(card->photo, data, datalen);
    }
}

void newsrc(enum eid_vwr_source new_source) {
    card_t *card = &global_card;

    if(new_source == EID_VWR_SRC_NONE) {
        printf("[+] card: cleaning card object\n");
        free(card->niss);
        free(card->cardid);
        free(card->photo);
        json_decref(card->fields);
        memset(card, 0x00, sizeof(card_t));
    }

    if(new_source == EID_VWR_SRC_CARD) {
        printf("[+] card: allocating new card\n");
        card->fields = json_object();
    }
}

void newstate(enum eid_vwr_states new_state) {
    switch(new_state) {
        case STATE_CALLBACKS:
            break;

        case STATE_READY:
            printf(BLUE("[+] event: ready, waiting card\n"));
            break;

        case STATE_TOKEN:
            printf("[+] event: new card found\n");
            break;

        case STATE_TOKEN_WAIT:
            printf("[+] event: card read, waiting for events\n");
            cardcommit();
            break;

        case STATE_TOKEN_ID:
            printf("[+] event: reading data from the card\n");
            break;

        case STATE_TOKEN_ERROR:
            printf("[-] event: card reading error\n");
            break;

        case STATE_CARD_INVALID:
            printf("[-] event: invalid card read\n");
            break;

        case STATE_NO_TOKEN:
            printf("[+] event: no card found\n");
            break;

        case STATE_NO_READER:
            printf("[+] event: reader not found, searching\n");
            break;

        case STATE_TOKEN_IDLE:
            // printf("idle\n");
            break;

        default:
            printf("[-] event: unhandled state: %d\n", new_state);
            break;
    }
}

int main(void) {
    struct eid_vwr_ui_callbacks *callbacks = eid_vwr_cbstruct();

    callbacks->newstate = newstate;
    callbacks->newstringdata = newstringdata;
    callbacks->newbindata = newbindata;
    callbacks->newsrc = newsrc;

    // eid_vwr_convert_set_lang(EID_VWR_LANG_EN);

    eid_vwr_createcallbacks(callbacks);

    while(1) {
        eid_vwr_poll();
        usleep(100000);
    }
}
