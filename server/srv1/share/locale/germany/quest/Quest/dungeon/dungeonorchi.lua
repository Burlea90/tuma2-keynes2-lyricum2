quest dungeonorchi begin
	state start begin
		function clear(arg)
			clear_server_timer("orchi_prepare", arg)
			clear_server_timer("orchi_end", arg)
			clear_server_timer("orchi_complete", arg)
			if d.find(arg) then
				d.setf(arg, "was_completed", 1)
				d.kill_all(arg)
				d.clear_regen(arg)
				d.exit_all_lobby(arg, 2560 + 764, 6656 + 723)
			end
		end

		when orchi_complete.server_timer begin
			dungeonorchi.clear(get_server_timer_arg())
		end

		when 693.kill with pc.in_dungeon() begin
			local idx = pc.get_map_index()
			if idx >= 3550000 and idx < 3560000 then
				if d.getf(idx, "floor") == 2 and d.getf(idx, "step") == 0 then
					d.setf(idx, "was_completed", 1)
					d.complete(693, 1800, 355, "dungeonorchi")
					if party.is_party() then
						notice_all(1272, party.get_leader_name())
					else
						notice_all(1273, pc.get_name())
					end
					
					d.notice(idx, 1054, "", true)
					d.notice(idx, 1055, "", true)
					d.kill_all(idx)
					d.clear_regen(idx)
					
					local bonus = 0 + game.get_event_flag("dungeon_bonus")
					if math.random(1, 100) <= bonus then
						d.spawn_mob(idx, 5001, 138, 885)
					end
					
					server_timer("orchi_complete", 30, idx)
				end
			end
		end

		when 8109.kill with pc.in_dungeon() begin
			local idx = pc.get_map_index()
			if idx >= 3550000 and idx < 3560000 then
				local f = d.getf(idx, "floor")
				if f == 2 then
					local s = d.getf(idx, "step") - 1
					if s == 0 then
						local try = d.set_vid_invincible(d.getf(idx, "boss"), false)
						if not try then
							d.notice(idx, 1050, "", true)
							dungeonorchi.clear(idx)
						else
							d.notice(idx, 1051, "", true)
						end
					else
						d.notice(idx, 1052, string.format("%d", s), true)
						d.notice(idx, 1053, "", true)
						local dmg = math.floor((6 - s) / 1.6)
						npc.set_vid_attack_mul(d.getf(idx, "boss"), dmg)
						npc.set_vid_damage_mul(d.getf(idx, "boss"), dmg)
					end
					
					d.setf(idx, "step", s)
				end
			end
		end

		when orchi_end.server_timer begin
			local arg = get_server_timer_arg()
			d.notice(arg, 1040, "", true)
			d.notice(arg, 1041, "", true)
			dungeonorchi.clear(arg)
		end

		when orchi_prepare.server_timer begin
			local arg = get_server_timer_arg()
			if d.find(arg) then
				d.setf(arg, "step", 4)
				d.spawn_mob(arg, 8109, 125, 892)
				d.spawn_mob(arg, 8109, 149, 892)
				d.spawn_mob(arg, 8109, 125, 915)
				d.spawn_mob(arg, 8109, 149, 915)
				local boss = d.spawn_mob(arg, 693, 138, 885)
				d.setf(arg, "boss", boss)
				local try = d.set_vid_invincible(boss, true)
				if not try then
					d.notice(arg, 1042, "", true)
					dungeonorchi.clear(arg)
				else
					d.notice(arg, 1128, string.format("%d", 30), true)
					server_timer("orchi_end", 1799, arg)
					d.notice(arg, 1046, "", true)
				end
			else
				dungeonorchi.clear(arg)
			end
		end

		when logout begin
			local idx = pc.get_map_index()
			if idx >= 3550000 and idx < 3560000 then
				pc.setf("dungeonorchi", "disconnect", get_global_time() + 300)
			end
		end

		when login begin
			local idx = pc.get_map_index()
			if idx == 355 then
				pc.warp((2560 + 764) * 100, (6656 + 723) * 100)
			elseif idx >= 3550000 and idx < 3560000 then
				pc.set_warp_location(64, 2560 + 764, 6656 + 723)
				pc.setf("dungeonorchi", "idx", idx)
				pc.setf("dungeonorchi", "ch", pc.get_channel_id())
				if d.getf(idx, "floor") == 0 then
					if not party.is_party() then
						d.setf(idx, "floor", 2)
						server_timer("orchi_prepare", 1, idx)
						d.setf(idx, "was_completed", 0)
					else
						if party.is_leader() then
							d.setf(idx, "floor", 2)
							server_timer("orchi_prepare", 1, idx)
							d.setf(idx, "was_completed", 0)
						end
					end
				end
			end
		end

		when 9239.chat."Labirintul Orcilor" begin
			local lang = pc.get_language()
			
			say_title(string.format("%s:", mob_name(9239)))
			say("")
			say(string.format(gameforge[lang].common.dungeon_1, pc.get_name()))
			local mapIdx = pc.get_map_index()
			if mapIdx != 64 then
				return
			end
			
			say(gameforge[lang].common.dungeon_2)
			local agree = select(gameforge[lang].common.yes, gameforge[lang].common.no)
			say_title(string.format("%s:", mob_name(9239)))
			say("")
			if agree == 2 then
				say(gameforge[lang].common.dungeon_3)
				return
			end
			
			local goahead = 1
			local rejoinTIME = pc.getf("dungeonorchi", "disconnect") - get_global_time()
			if rejoinTIME > 0 then
				local rejoinIDX = pc.getf("dungeonorchi", "idx")
				if rejoinIDX > 0 then
					local rejoinCH = pc.getf("dungeonorchi", "ch")
					if rejoinCH != 0 and rejoinCH != pc.get_channel_id() then
						say(string.format(gameforge[lang].common.dungeon_26, rejoinCH))
						return
					end
					
					if rejoinCH != 0 and d.find(rejoinIDX) then
						if d.getf(rejoinIDX, "was_completed") == 0 then
							say(gameforge[lang].common.dungeon_4)
							local agree2 = select(gameforge[lang].common.yes, gameforge[lang].common.no)
							if agree2 == 2 then
								say_title(string.format("%s:", mob_name(9239)))
								say("")
								say(gameforge[lang].common.dungeon_3)
								return
							end
							
							local f = d.getf(rejoinIDX, "floor")
							if f == 2 then
								goahead = 0
								pc.warp(218600, 348900, rejoinIDX)
							end
						end
					end
				end
			end
			
			if goahead == 1 then
				pc.setf("dungeonorchi", "disconnect", 0)
				pc.setf("dungeonorchi", "idx", 0)
				pc.setf("dungeonorchi", "ch", 0)
				
				say(gameforge[lang].common.dungeon_5)
				say_reward(string.format(gameforge[lang].common.dungeon_6, 30))
				say_reward(string.format(gameforge[lang].common.dungeon_7, 60))
				say_reward(string.format("- %s: %d", item_name(89106), 1))
				if party.is_party() then
					say_reward(gameforge[lang].common.dungeon_8)
				end
				say(gameforge[lang].common.dungeon_9)
				local n = select(gameforge[lang].common.yes, gameforge[lang].common.no)
				say_title(string.format("%s:", mob_name(9239)))
				say("")
				if n == 2 then
					say(gameforge[lang].common.dungeon_3)
					return
				end
				
				local flag = string.format("ww_355_%d", pc.get_channel_id())
				local ww_flag = game.get_event_flag(flag) - get_global_time()
				if ww_flag > 0 then
					say(gameforge[lang].common.dungeon_28)
					say(string.format(gameforge[lang].common.dungeon_29, ww_flag))
					return
				end
				
				myname = pc.get_name()
				result, cooldown, name = d.check_entrance(30, 60, 89106, 0, 1, "dungeonorchi.cooldown")
				if result == 0 then
					say(gameforge[lang].common.unknownerr)
				elseif result == 2 then
					say(gameforge[lang].common.dungeon_10)
				elseif result == 3 then
					if myname == name then
						say(gameforge[lang].common.dungeon_20)
					else
						say(string.format(gameforge[lang].common.dungeon_19, name))
					end
				elseif result == 4 then
					if myname == name then
						say(gameforge[lang].common.dungeon_12)
					else
						say(string.format(gameforge[lang].common.dungeon_11, name))
					end
				elseif result == 5 then
					if myname == name then
						say(gameforge[lang].common.dungeon_14)
					else
						say(string.format(gameforge[lang].common.dungeon_13, name))
					end
				elseif result == 6 or result == 7 then
					if myname == name then
						say(string.format(gameforge[lang].common.dungeon_18, 1, item_name(89106)))
					else
						say(string.format(gameforge[lang].common.dungeon_17, name, 1, item_name(89106)))
					end
				elseif result == 8 then
					local h = math.floor(cooldown / 3600)
					local m = math.floor((cooldown - (3600 * h)) / 60)
					local hS = gameforge[lang].common.hours
					if h == 1 then
						hS = gameforge[lang].common.hour
					end
					local mS = gameforge[lang].common.minutes
					if m == 1 then
						mS = gameforge[lang].common.minute
					end
					
					if myname == name then
						say(string.format(gameforge[lang].common.dungeon_16, h, hS, m, mS))
					else
						say(string.format(gameforge[lang].common.dungeon_15, name, h, hS, m, mS))
					end
				elseif result == 1 then
					say(gameforge[lang].common.dungeon_21)
					wait()
					d.remove_item(89106, 1, 0, 0, 89104, 255, 89105, 255, 0, 0, 0, 0, 0, 0, 0, 0, 1800, "dungeonorchi")
					game.set_event_flag(string.format("ww_356_%d", pc.get_channel_id()), get_global_time() + 5)
					d.join_cords(355, 2186, 3489)
				end
			end
		end
	end
end

