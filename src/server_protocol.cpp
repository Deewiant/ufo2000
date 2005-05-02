/*
This file is part of "UFO 2000" aka "X-COM: Gladiators"
                    http://ufo2000.sourceforge.net/
Copyright (C) 2000-2001  Alexander Ivanov aka Sanami
Copyright (C) 2002-2003  ufo2000 development team

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdio.h>
#include <stdexcept>

#include "server_protocol.h"
#include "server_config.h"
#include "server_game.h"

std::string ServerClientUfo::m_last_user_name = "";
NLtime      ServerClientUfo::m_last_user_disconnect_time;
int         ServerClientUfo::m_games_started = 0;

static std::string time_to_string(long t)
{
    char buffer[64];
    long days    = t / (24 * 3600 * 1000); t -= days * (24 * 3600 * 1000);
    long hours   = t / (3600 * 1000); t -= hours * (3600 * 1000);
    long minutes = t / (60 * 1000); t -= minutes * (60 * 1000);
    long seconds = t / 1000;

    if (days > 0)
        sprintf(buffer, "%ldd %02ldh %02ldm %02lds", days, hours, minutes, seconds);
    else if (hours > 0)
        sprintf(buffer, "%02ldh %02ldm %02lds", hours, minutes, seconds);
    else if (minutes > 0)
        sprintf(buffer, "%02ldm %02lds", minutes, seconds);
    else
        sprintf(buffer, "%lds", seconds);

    return buffer;
}

static std::string num_to_string(int x)
{
    char buffer[32];
    sprintf(buffer, "%d", x);
    return buffer;
}

void ServerDispatch::MakeHtmlReport(std::string &html_body)
{
    NLtime now;
    nlTime(&now);

    long traffic_in_cur = 0;
    long traffic_out_cur = 0;

    html_body = "<html><head><title>UFO2000 Matchmaker</title></head><body>";
    html_body += "<h3>UFO2000 matchmaking server.</h3><blockquote>";
    html_body += "<a href='http://ufo2000.sourceforge.net/'>Project home page (and the latest stable version download link)</a><br>";
    html_body += "<a href='http://lxnt.info/mailman/listinfo/ufo2000'>Developers mailing list</a><br>";
    html_body += "<a href='http://www.xcomufo.com/forums/index.php?showforum=266'>Official Forum</a><br>";
    html_body += "<br><a href='http://ufo2000.lxnt.info/pmwiki/index.php/Main/DevelopmentVersion'>Latest beta versions of the game</a><br>";
    html_body += "<a href='http://ufo2000.lxnt.info/pmwiki/index.php/Main/NewMaps'>Additional user made maps</a><br>";
    html_body += "<a href='http://ufo2000.lxnt.info/results.php'>Official server battle statistics</a><br>";
    html_body += "<br></blockquote><hr><table border=1>";
    html_body += "<tr><td>user name<td>bytes from<td>bytes to<td>max average traffic<td>time online<td>status";

//  Report other players status
    std::map<std::string, ServerClient *>::iterator it = m_clients_by_name.begin();
    while (it != m_clients_by_name.end()) {
        ServerClientUfo *client = dynamic_cast<ServerClientUfo *>(it->second);

        html_body += "<tr><td>";
        html_body += client->m_name;
        html_body += "<td>";
        html_body += num_to_string(nlGetSocketStat(client->m_socket, NL_BYTES_RECEIVED));
        traffic_in_cur += nlGetSocketStat(client->m_socket, NL_BYTES_RECEIVED);
        html_body += "<td>";
        html_body += num_to_string(nlGetSocketStat(client->m_socket, NL_BYTES_SENT));
        traffic_out_cur += nlGetSocketStat(client->m_socket, NL_BYTES_SENT);
        html_body += "<td>";
        html_body += num_to_string(client->m_max_ave_traffic);
        html_body += "<td>";
        html_body += time_to_string(get_time_diff(client->m_connection_time, now));
        html_body += "<td>";
        if (client->game) {
            char tmp_buff[100];
            sprintf(tmp_buff, "in game %d", client->game->game_id);
            html_body += tmp_buff;
        } else if (client->is_in_server_chat()) {
            html_body += "in server chat";
        } else {
            html_body += "busy";
        }
        it++;
    }

    html_body += "</table>";
    html_body += "<br>";

    if (m_clients_by_name.size() == 0 && !ServerClientUfo::m_last_user_name.empty()) {
        html_body += "the last user ";
        html_body += ServerClientUfo::m_last_user_name;
        html_body += " was here ";
        html_body += time_to_string(get_time_diff(ServerClientUfo::m_last_user_disconnect_time, now));
        html_body += " ago<br><br>";
    }

    html_body += "game server uptime = ";
    html_body += time_to_string(get_time_diff(m_connection_time, now)) + "<br>";
    html_body += "number of games started up to this moment = ";
    html_body += num_to_string(ServerClientUfo::m_games_started) + "<br>";
    html_body += "total incoming game traffic = ";
    html_body += num_to_string(m_traffic_in + traffic_in_cur) + "<br>";
    html_body += "total outcoming game traffic = ";
    html_body += num_to_string(m_traffic_out + traffic_out_cur) + "<br>";
    html_body += "total incoming HTTP traffic = ";
    html_body += num_to_string(m_http_traffic_in) + "<br>";
    html_body += "total outcoming HTTP traffic = ";
    html_body += num_to_string(m_http_traffic_out) + "<br>";
    html_body += "</body></html>";
}

ServerClient *ServerDispatch::CreateServerClient(NLsocket socket)
{
    return new ServerClientUfo(this, socket);
}

ServerClientUfo::~ServerClientUfo()
{
    if(game)
        Server_Game_UFO::DeactivatePlayer(this);
        
    if (m_name != "") {
    //  send information that the user is offline to all other users
        send_packet_all(SRV_USER_OFFLINE, m_name);

    //  remove this user from challenge lists of other players
        std::map<std::string, ServerClient *>::iterator it = m_server->m_clients_by_name.begin();
        while (it != m_server->m_clients_by_name.end()) {
            ServerClientUfo *opponent = dynamic_cast<ServerClientUfo *>(it->second);
            opponent->m_challenged_opponents.erase(m_name);
            it++;
        }

    //  Save the name and disconnect time of user
        m_last_user_name = m_name;
        nlTime(&m_last_user_disconnect_time);

    }
}

bool ServerClientUfo::recv_packet(NLulong id, const std::string &packet)
{
    if(game)
        game->PacketToServer(this, id, packet);
    if ((id == SRV_GAME_PACKET) && (strstr(packet.c_str(), "_Xmes_") != NULL))
        server_log("packet from %s {game chat message}\n", m_name.c_str(), (int)id);
    else
        server_log("packet from %s {id=%d, data=%s}\n", m_name.c_str(), (int)id, packet.c_str());

//  only SRV_LOGIN packet is accepted from not authenticated users
    if (m_name == "" && id != SRV_LOGIN) {
        m_error = true;
        return false;
    }

//  process incoming packet
    switch (id) {
        case SRV_LOGIN: {
            std::string login, password;
            split_loginpass(packet, login, password);

            server_log("user login (name='%s', pwd='%s', ip=%s)\n",
                login.c_str(), password.c_str(), m_ip.c_str());

            if (login.size() > USERNAME_SIZE_LIMIT) {
                server_log("login failed: user name is too long\n");
                send_packet_back(SRV_FAIL, "User name is too long");
                m_error = true;
                break;
            } else if (login.size() == 0) {
                server_log("login failed: user name is too short\n");
                send_packet_back(SRV_FAIL, "User name is too short");
                m_error = true;
                break;
            }
            
            int validate_user_result = validate_user(login, password);
            if (validate_user_result < 0) {
                server_log("login failed: invalid password\n");
                send_packet_back(SRV_FAIL, "Login failed (invalid password)");
                m_error = true;
                break;
            } else if (validate_user_result == 0) {
                if (password.size() >= 6 && add_user(login, password)) {
                    server_log("successful registration: %s:%s\n", login.c_str(), password.c_str());
                } else {
                    server_log("registration failed: password is too short\n");
                    send_packet_back(SRV_FAIL, "Registration failed (password is too short)");
                    m_error = true;
                    break;
                }
            }

            if (m_server->m_clients_by_name.find(login) != m_server->m_clients_by_name.end()) {
                server_log("login failed: already online\n");
                send_packet_back(SRV_FAIL, "User with this name is already online");
                m_error = true;
                break;
            }

        //  check clients number limit
            if (m_server->m_clients_by_name.size() >= PLAYERS_COUNT_LIMIT) {
                server_log("login failed: too many players\n");
                send_packet_back(SRV_FAIL, "Too many players on server");
                m_error = true;
                break;
            }

            server_log("login ok\n");

            m_name = login;
            send_packet_back(SRV_OK, "login ok");
        // send user list to a newly created user
            std::map<std::string, ServerClient *>::iterator it = m_server->m_clients_by_name.begin();
            while (it != m_server->m_clients_by_name.end()) {
                ServerClientUfo *opponent = dynamic_cast<ServerClientUfo *>(it->second);

                printf("send user online: %s\n", opponent->m_name.c_str());
                opponent->send_packet_back(SRV_USER_ONLINE, m_name);
                if (opponent->m_busy)
                    send_packet_back(SRV_USER_BUSY, opponent->m_name);
                else
                    send_packet_back(SRV_USER_ONLINE, opponent->m_name);
                it++;
            }

        //  If we are alone on the server, show information about the last user disconnected
            if (m_server->m_clients_by_name.size() == 0 && !m_last_user_name.empty()) {
                NLtime now;
                nlTime(&now);
                std::string message;
                send_packet_back(SRV_MESSAGE, "server: There are no other users connected now");
                send_packet_back(SRV_MESSAGE, "server: You have to wait for someone else before you can play games");
                message += "server: ";
                message += m_last_user_name;
                message += " was here ";
                message += time_to_string(get_time_diff(m_last_user_disconnect_time, now));
                message += " ago";
                send_packet_back(SRV_MESSAGE, message);
            }

            m_server->m_clients_by_name[m_name] = this;
            break;
        }
        case SRV_CHALLENGE: {
        //  Check that the opponent is currently online
            std::map<std::string, ServerClient *>::iterator it = m_server->m_clients_by_name.find(packet);
            if (it == m_server->m_clients_by_name.end()) {
                printf("Warning: opponent '%s' is offline\n", packet.c_str());
                send_packet_back(SRV_USER_OFFLINE, packet);
                break;
            }

            ServerClientUfo *opponent = dynamic_cast<ServerClientUfo *>(it->second);

        //  Check that the opponent is not busy now
            if (opponent->m_busy) {
                printf("Warning: opponent '%s' is busy\n", packet.c_str());
                send_packet_back(SRV_USER_BUSY, packet);
                break;
            }

        //  Try to find self in the opponent's challenge list
            if (opponent->m_challenged_opponents.find(m_name) != opponent->m_challenged_opponents.end()) {
            //  opponent found in the challenge list
                long int game_id = Server_Game_UFO::CreateGame(opponent->m_name, m_name);
                Server_Game_UFO::ActivatePlayer(game_id, this);
                Server_Game_UFO::ActivatePlayer(game_id, opponent);

                server_log("game start: '%s' vs '%s'\n", m_name.c_str(), packet.c_str());
                m_games_started++;

                send_packet_all(SRV_USER_BUSY, m_name);
                opponent->send_packet_all(SRV_USER_BUSY, packet);
                send_packet_back(SRV_GAME_START_JOIN, packet);
                opponent->send_packet_back(SRV_GAME_START_HOST, m_name);
                m_busy = true;
                opponent->m_busy = true;
                m_challenged_opponents.clear();
                opponent->m_challenged_opponents.clear();
            } else {
            //  insert the opponent into challenge list
                m_challenged_opponents.insert(packet);
                opponent->send_packet_back(SRV_USER_CHALLENGE_IN, m_name);
                send_packet_back(SRV_USER_CHALLENGE_OUT, packet);
            }
            break;
        }
        case SRV_MESSAGE: {
        // send message to all other logged in users
            send_packet_all(SRV_MESSAGE, m_name + ": " + packet);
            break;
        }
        case SRV_ENDGAME: {
            m_busy = false;

        //  Report other players status
            std::map<std::string, ServerClient *>::iterator it = m_server->m_clients_by_name.begin();
            while (it != m_server->m_clients_by_name.end()) {
                ServerClientUfo *opponent = dynamic_cast<ServerClientUfo *>(it->second);
                if (opponent->m_name != m_name) {
                    opponent->send_packet_back(SRV_USER_ONLINE, m_name);
                    if (opponent->m_busy)
                        send_packet_back(SRV_USER_BUSY, opponent->m_name);
                    else
                        send_packet_back(SRV_USER_ONLINE, opponent->m_name);
                }
                it++;
            }

            break;
        }
        case SRV_GAME_REPLAY_REQUEST: {
	        send_packet_back(SRV_GAME_RECOVERY_START, "1");
            try {
                sqlite3::reader reader=db_conn.executereader("select command, packet_type, id from ufo2000_game_packets where game=%s order by id;", packet.c_str());
			    int game_start_sended = 0;
                while(reader.read()) {
	                if(reader.getint32(1) == SRV_GAME_PACKET) {
                        if(!(reader.getstring(0) == "START" && game_start_sended))
                            send_packet_back(SRV_GAME_PACKET, reader.getstring(0));
	                    if(reader.getstring(0) == "START") {
                            game_start_sended = 1;
                            send_packet_back(SRV_GAME_PACKET, "_Xcom_2_VSRC_");
                        }
		            }
                }
				reader.close();
			} catch(std::exception &ex) {
                server_log("Exception Occured: %s",ex.what());
            }
			break;
        }
        case SRV_GAME_CONTINUE_REQUEST: {
            int game_id = db_conn.executeint32("select max(game) from ufo2000_game_players where player='%s';", m_name.c_str());
			if(game_id > 0) {
				Server_Game_UFO::ActivatePlayer(game_id, this);
				int players_position = db_conn.executeint32("select position from ufo2000_game_players where player='%s' and game=%d;", m_name.c_str(), game_id);
				int last_sended_packet = db_conn.executeint32("select last_sended_packet from ufo2000_game_players where player='%s' and game=%d;", m_name.c_str(), game_id);
				char pos_str_buffer[100];
	            sprintf(pos_str_buffer, "%d", players_position);
		        send_packet_back(SRV_GAME_RECOVERY_START, pos_str_buffer);
			    sqlite3::reader reader=db_conn.executereader("select command, packet_type, id from ufo2000_game_packets where game=%d order by id;", game_id);
				int game_start_sended = 0;
	            while(reader.read())
		            if(reader.getint32(1) == SRV_GAME_PACKET)
			        {
				        if(!(reader.getstring(0) == "START" && game_start_sended))
					        send_packet_back(SRV_GAME_PACKET, reader.getstring(0));
	                    if(reader.getstring(0) == "START")
		                    game_start_sended = 1;
			            if(reader.getint32(2) == last_sended_packet) {
				            char start_visible_packet[100];
					        sprintf(start_visible_packet, "_Xcom_%d_VSRC_", 3 - players_position);
						    send_packet_back(SRV_GAME_PACKET, start_visible_packet);
	                    }
		            }
				reader.close();
	            char stop_packet[100];
		        sprintf(stop_packet, "_Xcom_%d_RSTP_", 3 - players_position);
			    send_packet_back(SRV_GAME_PACKET, stop_packet);
			}
			break;
        }
    }
    return true;
}

bool ClientServerUfo::login(const std::string &name, const std::string &pass,
    std::string &error_message)
{
    if (!send_packet(SRV_LOGIN, name + ":" + pass)) {
        error_message = "Failed to connect";
        return false;
    }

    NLulong id;
    if (wait_packet(id, error_message) == -1) {
        error_message = "Failed to connect";
        return false;
    }

    return id == SRV_OK;
};

bool ClientServerUfo::message(const std::string &text)
{
    return send_packet(SRV_MESSAGE, text);
}

bool ClientServerUfo::challenge(const std::string &user)
{
    if (!send_packet(SRV_CHALLENGE, user)) return false;
    return true;
}

bool ClientServerUfo::decline_challenge(const std::string &user)
{
    if (!send_packet(SRV_DECLINE_CHALLENGE, user)) return false;
    return true;
}

bool ClientServerUfo::resume_game()
{
    if (!send_packet(SRV_GAME_CONTINUE_REQUEST , "Game request")) return false;
    return true;
}

bool ClientServerUfo::resume_game_debug(std::string game_id)
{
    if (!send_packet(SRV_GAME_REPLAY_REQUEST , game_id)) return false;
    return true;
}

bool ServerClientUfo::add_user(const std::string &login, const std::string &password)
{
    try {
        db_conn.executenonquery("insert into ufo2000_users(name,password) values('%s','%s');", login.c_str(),password.c_str());
    }
    catch(std::exception &ex) {
        server_log("Exception Occured: %s",ex.what());
    }
    return true;
}

/**
 * @return  0 - not registered\n
 *          1 - password valid\n
 *         -1 - password invalid
 */
int ServerClientUfo::validate_user(const std::string &username, const std::string &password)
{
    try {
    if (!db_conn.executeint32("select count(*) from ufo2000_users where name='%s';",username.c_str()))
        return 0;
    if (db_conn.executeint32("select count(*) from ufo2000_users where name='%s' and password='%s';",username.c_str(),password.c_str()))
        return 1;
    }
    catch(std::exception &ex) {
        server_log("Exception Occured: %s",ex.what());
    }
    return -1;
    
/*    sqlite3::reader reader=con.executereader("select * from t_test;");
    if (accept_user.find(username) == accept_user.end()) return 0;
    return accept_user[username] == password ? 1 : -1;*/
}

