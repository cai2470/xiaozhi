idf_component_register(SRCS
                        "main.c"
                        "Dri/Driver_WIFI.c"
                        "Dri/Driver_HttpClient.c"
                        "Dri/Driver_WebSocket.c"
                        "Inf/Inf_key.c"
                        "Inf/Inf_ES8311.c"
                        "Inf/Inf_SR.c"
                        "Inf/Inf_Encoder.c"
                        "Inf/Inf_Decoder.c"
                        "Inf/Inf_Lcd.c"
                        "App/App_Application.c"
                        "App/App_Audio.c"
                        "App/App_OTA.c"
                        "App/App_Communication.c"
                        "App/App_Display.c"
                        "Common/Common_Config.c"
                        "Common/Common_Debug.c"


                    INCLUDE_DIRS
                       "App"
                       "Common"
                       "Dri"
                       "Inf"
                       "."
                   
                    )
