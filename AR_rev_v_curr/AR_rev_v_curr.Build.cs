// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class AR_rev_v_curr : ModuleRules
{
	public AR_rev_v_curr(TargetInfo Target)
	{
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "OnlineSubsystem", "OnlineSubsystemUtils" });
        DynamicallyLoadedModuleNames.Add("OnlineSubsystemNull");
        DynamicallyLoadedModuleNames.Add("OnlineSubsystemSteam");


        //PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "OnlineSubsystem", "OnlineSubsystemUtils" });
        //DynamicallyLoadedModuleNames.Add("OnlineSubsystemNull");

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");
        // if ((Target.Platform == UnrealTargetPlatform.Win32) || (Target.Platform == UnrealTargetPlatform.Win64))
        // {
        //		if (UEBuildConfiguration.bCompileSteamOSS == true)
        //		{
        //			DynamicallyLoadedModuleNames.Add("OnlineSubsystemSteam");
        //		}
        // }
    }
}
