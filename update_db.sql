CREATE TABLE ufo2000_users (name text primary key,password text);
CREATE TABLE ufo2000_sequences (name text primary key,seq_val integer);
CREATE TABLE ufo2000_games (id integer primary key,  last_received_packed integer, is_finished text, errors text, result integer);
CREATE TABLE ufo2000_game_players(game integer,  player text, last_sended_packet integer, position integer, primary key(game, player));
insert or ignore into ufo2000_sequences values ('ufo2000_games',0);
CREATE TABLE ufo2000_game_packets(game integer, id integer, sender integer, date text, command text, packet_type integer, primary key(game,id));
CREATE TRIGGER ufo2000_game_insert insert ON ufo2000_games BEGIN delete from ufo2000_game_packets where game<new.id-100; END;
pragma autovacuum=on;