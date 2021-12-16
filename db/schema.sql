CREATE TABLE cards (niss varchar(32), cardid varchar(32), fields text, photo blob, added datetime default current_timestamp);
CREATE INDEX cards_cardid on cards(cardid);
CREATE INDEX cards_niss on cards(niss);

