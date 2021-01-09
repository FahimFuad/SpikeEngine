/*
                               SPIKE ENGINE
               This file is licensed under the SPIKE LICENSE
                          --Spike Source Code--

         Copyright 2021 - SpikeTechnologies - All Rights Reserved

File Name      : EntryPoint
File Type      : h
File created on: 2021/01/09
File created by: Fahim Fuad
Other editors  : None
Github repository : https://github.com/FahimFuad/Spike
 
0.This software is provided 'AS-IS', without any express or implied warranty.
  In no event will the authors or contributors be held liable for any damages
  arising from the use of this software.

1.The origin of this software must not be misrepresented; you must not claim
  that you wrote the original software.
 
2.You MUST NOT change or alter this file. This excludes the contributions done
  by people. Changing this file is PERFECTLY LEGAL if you are contributing.

3. THIS NOTICE MAY NOT BE REMOVED OR ALTERED FROM ANY SOURCE DISTRIBUTION.
*/
#pragma once

#ifdef SPK_PLATFORM_WINDOWS

extern Spike::Application* Spike::CreateApplication();

int main(int argc, char** argv)
{
    Spike::Log::Init();
    LE_PROFILE_BEGIN_SESSION("Startup", "SpikeProfile-Startup.json");
    auto app = Spike::CreateApplication();
    LE_PROFILE_END_SESSION();

    LE_PROFILE_BEGIN_SESSION("Runtime", "SpikeProfile-Runtime.json");
    app->Run();
    LE_PROFILE_END_SESSION();

    LE_PROFILE_BEGIN_SESSION("Shutdown", "SpikeProfile-Shutdown.json");
    delete app;
    LE_PROFILE_END_SESSION();
}

#endif // SPK_PLATFORM_WINDOWS
