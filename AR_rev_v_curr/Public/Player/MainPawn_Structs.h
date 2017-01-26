#pragma once

#include "MainPawn_Enums.h"
#include "MainPawn_Structs.generated.h"


/*
Struct to send Player-Input-Data (Compressed to 64 bit) to server:
Movement-Input: Forward/Side [left,right,forward,backwards]
Cumulative Mouse-Input: Up/Right [-1.0..1.0]
Packet-Number
Gun-Button pressed: [true,false]
Missile-Button pressed: [true,false]
Target-Lock-Button pressed: [true,false]
*/
USTRUCT()
struct FPlayerInputPackage {
	GENERATED_USTRUCT_BODY()
		// Default Constructor
	FPlayerInputPackage();

	// Constructor to initialize the Struct
	FPlayerInputPackage(bool Gun, bool Missile, bool Lock, const FVector2D& MovementInput, const FVector2D& MouseInput, const uint32 newPacketNumber);;

	/* TODO */
	UPROPERTY()
		uint64 PlayerDataCompressed;

	// 4 bit 64-4 = 60
	void SetMovementInput(const FVector2D& MovementInput);

	FVector2D GetMovementInput() const;

	// 24 bit 60-24 = 36
	void setMouseInput(const FVector2D& MouseInput);

	float getMouseYaw() const;

	float getMousePitch() const;

	FVector2D getMouseInput() const;

	void setPacketNumber(const uint32 newPacketNumber);

	void IncrementPacketNumber();

	// 24 bit 36-24 = 12
	uint32 getPacketNumber() const;

	// 1 bit 64th  11
	void setGunFire(const uint32 bGunFire);

	bool getGunFire() const;

	// 1 bit 63th 10
	void setMissileFire(const uint32 bMissileFire);

	bool getMissileFire() const;

	// help function to reduce redundancy
	void setBit(const uint32 bSet, const int Position);

	// 1 bit 62th 9	
	void setSwitchTarget(const uint32 bSwitchTarget);

	bool getSwitchTarget() const;

	// 1 bit 61th 8
	void setGunLock(const uint32 bGunLock);

	bool getGunLock() const;

	// 1 bit 60th 7
	void setMissileLock(const uint32 bMissileLock);

	bool getMissileLock() const;
};

inline FPlayerInputPackage::FPlayerInputPackage()
{
	PlayerDataCompressed = 0LLU;
}

inline FPlayerInputPackage::FPlayerInputPackage(bool Gun, bool Missile, bool Lock, const FVector2D& MovementInput, const FVector2D& MouseInput, const uint32 newPacketNumber)
{
	PlayerDataCompressed = 0LLU;
	setGunFire(Gun);
	setMissileFire(Missile);
	setSwitchTarget(Lock);
	SetMovementInput(MovementInput);
	setMouseInput(MouseInput);
	setPacketNumber(newPacketNumber);
}

inline void FPlayerInputPackage::SetMovementInput(const FVector2D& MovementInput)
{
	uint64 const rightPressed = 1LLU;
	uint64 const leftPressed = 1LLU << 1;
	uint64 const forwardPressed = 1LLU << 2;
	uint64 const backwardsPressed = 1LLU << 3;
	uint64 const clearMovementInput = ~15LLU;

	PlayerDataCompressed &= clearMovementInput;
	// forward
	if (MovementInput.X > 0.0f)
	{
		PlayerDataCompressed |= forwardPressed;
	}
	else
	// backwards
	if (MovementInput.X < 0.0f)
	{
		PlayerDataCompressed |= backwardsPressed;
	}
	// right
	if (MovementInput.Y > 0.0f)
	{
		PlayerDataCompressed |= rightPressed;
	}
	else
	// left
	if (MovementInput.Y < 0.0f)
	{
		PlayerDataCompressed |= leftPressed;
	}
}

inline FVector2D FPlayerInputPackage::GetMovementInput() const
{
	uint64 const rightPressed = 1LLU;
	uint64 const leftPressed = 1LLU << 1;
	uint64 const forwardPressed = 1LLU << 2;
	uint64 const backwardsPressed = 1LLU << 3;
	FVector2D Input = FVector2D::ZeroVector;
	// forward
	if ((PlayerDataCompressed & forwardPressed) == forwardPressed)
	{
		Input.X = 1.0f;
	}
	// backwards
	if ((PlayerDataCompressed & backwardsPressed) == backwardsPressed)
	{
		Input.X = -1.0f;
	}
	// right
	if ((PlayerDataCompressed & rightPressed) == rightPressed)
	{
		Input.Y = 1.0f;
	}
	// left
	if ((PlayerDataCompressed & leftPressed) == leftPressed)
	{
		Input.Y = -1.0f;
	}
	return Input;
}

inline void FPlayerInputPackage::setMouseInput(const FVector2D& MouseInput)
{
	uint64 const mouseInputClear = ~(16777215LLU << 4);
	uint64 const mouseYawIsNegative = 1LLU << 15;
	uint64 const mousePitchIsNegative = 1LLU << 27;

	PlayerDataCompressed &= mouseInputClear;

	PlayerDataCompressed |= ((uint64)(2047 * FMath::Abs(MouseInput.X))) << 4;
	if (MouseInput.X < 0.0f)
	{
		PlayerDataCompressed |= mouseYawIsNegative;
	}

	PlayerDataCompressed |= ((uint64)(2047 * FMath::Abs(MouseInput.Y))) << 16;
	if (MouseInput.Y < 0.0f)
	{
		PlayerDataCompressed |= mousePitchIsNegative;
	}
}

inline float FPlayerInputPackage::getMouseYaw() const
{
	uint64 const mouseYaw = 2047LLU << 4;
	uint64 const mouseYawIsNegative = 1LLU << 15;

	if ((PlayerDataCompressed & mouseYawIsNegative) == mouseYawIsNegative)
	{
		return 0.0f - (((PlayerDataCompressed & mouseYaw) >> 4) / 2047.0f);
	}
	return ((PlayerDataCompressed & mouseYaw) >> 4) / 2047.0f;
}

inline float FPlayerInputPackage::getMousePitch() const
{
	uint64 const mousePitch = 2047LLU << 16;
	uint64 const mousePitchIsNegative = 1LLU << 27;

	if ((PlayerDataCompressed & mousePitchIsNegative) == mousePitchIsNegative)
	{
		return 0.0f - (((PlayerDataCompressed & mousePitch) >> 16) / 2047.0f);
	}
	return ((PlayerDataCompressed & mousePitch) >> 16) / 2047.0f;
}

inline FVector2D FPlayerInputPackage::getMouseInput() const
{
	return FVector2D(getMouseYaw(), getMousePitch());
}

inline void FPlayerInputPackage::setPacketNumber(const uint32 newPacketNumber)
{
	uint64 const PacketNoClear = ~(16777215LLU << 28);
	PlayerDataCompressed &= PacketNoClear;
	PlayerDataCompressed |= ((uint64)newPacketNumber) << 28;
}

inline void FPlayerInputPackage::IncrementPacketNumber()
{
	setPacketNumber(1 + getPacketNumber());
}

inline uint32 FPlayerInputPackage::getPacketNumber() const
{
	uint64 const PacketNo = 16777215LLU << 28;
	return (uint32)((PlayerDataCompressed & PacketNo) >> 28);
}

inline void FPlayerInputPackage::setGunFire(const uint32 bGunFire)
{
	setBit(bGunFire, 63);
}

inline bool FPlayerInputPackage::getGunFire() const
{
	return (PlayerDataCompressed & (1LLU << 63)) == (1LLU << 63);
}

inline void FPlayerInputPackage::setMissileFire(const uint32 bMissileFire)
{
	setBit(bMissileFire, 62);
}

inline bool FPlayerInputPackage::getMissileFire() const
{
	return (PlayerDataCompressed & (1LLU << 62)) == (1LLU << 62);
}

inline void FPlayerInputPackage::setBit(const uint32 bSet, const int Position)
{
	if (bSet)
	{
		PlayerDataCompressed |= (1LLU << Position);
	}
	else
	{
		PlayerDataCompressed &= ~(1LLU << Position);
	}
}

inline void FPlayerInputPackage::setSwitchTarget(const uint32 bSwitchTarget)
{
	setBit(bSwitchTarget, 61);
}

inline bool FPlayerInputPackage::getSwitchTarget() const
{
	return (PlayerDataCompressed & (1LLU << 61)) == (1LLU << 61);
}

inline void FPlayerInputPackage::setGunLock(const uint32 bGunLock)
{
	setBit(bGunLock, 60);
}

inline bool FPlayerInputPackage::getGunLock() const
{
	return (PlayerDataCompressed & (1LLU << 60)) == (1LLU << 60);
}

inline void FPlayerInputPackage::setMissileLock(const uint32 bMissileLock)
{
	setBit(bMissileLock, 59);
}

inline bool FPlayerInputPackage::getMissileLock() const
{
	return (PlayerDataCompressed & (1LLU << 59)) == (1LLU << 59);
}

/* Struct that stores the current Pawn-Transform with the corresponding Packet-Number of the Player-Input.
Used to see how much the client has desynchronized when an accepted Player-Input is received from the server */
USTRUCT()
struct FPositionHistoryElement {
	GENERATED_USTRUCT_BODY()

		/* Current Player-Input-Packet-Number */
		UPROPERTY()
		uint16 PacketNo;

	/* Corresponding Transform of the Pawn in Time when Player-Input with PacketNo was created */
	UPROPERTY()
		FTransform Transform;
};

USTRUCT(BlueprintType)
struct FTarget {
	GENERATED_USTRUCT_BODY()
		
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
		AActor* Actor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
		FTransform PreviousTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
		FTransform CurrentTransform;
};
