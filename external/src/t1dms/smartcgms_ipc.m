function result = smartcgms_ipc(in)

glucose_ig = in(1);
glucose_bg = in(2);
insulin_injection = in(3);
time = in(4);

[basalrate, bolusrate, carbsrate, device_time] = gpredict3_controller(glucose_ig, glucose_bg, insulin_injection, time);

result = [ basalrate bolusrate carbsrate device_time ];
