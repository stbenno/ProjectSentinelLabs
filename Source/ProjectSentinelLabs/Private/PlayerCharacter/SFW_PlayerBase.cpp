// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter/SFW_PlayerBase.h"
#include "Net/UnrealNetwork.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Core/Actors/Interface/SFW_InteractableInterface.h"
#include "Core/Components/SFW_EquipmentManagerComponent.h"
#include "Core/Components/SFW_EquipmentManagerComponent.h"



// Sets default values
ASFW_PlayerBase::ASFW_PlayerBase()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	//bReplicateMovement = true;

	//Capsule

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	// FPS style rotation
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	//Movement Speed
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	bIsCrouched = false;

	//Enable Crouch
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	//GetCharacterMovement()->CrouchedHalfHeight = 60.f; //default 60, may tweak if needed

	//Camera
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->bUsePawnControlRotation = true;
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));

	//Use built in mesh as 3P full body (what other players will see)
	USkeletalMeshComponent* Mesh3P = GetMesh();
	Mesh3P->SetRelativeLocation(FVector(0.f, 0.f, -96.f)); //align to capsule
	Mesh3P->SetOwnerNoSee(true);
	Mesh3P->SetCastHiddenShadow(true);

	//FirstPerson Arms (local)

	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh1P_Arms"));
	Mesh1P->SetupAttachment(FirstPersonCamera);
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// ...
	EquipmentManager = CreateDefaultSubobject<USFW_EquipmentManagerComponent>(TEXT("EquipmentManager"));

}

// Called when the game starts or when spawned
void ASFW_PlayerBase::BeginPlay()
{
	Super::BeginPlay();
	UpdateMeshVisibility();

	// Init speed from current state
	ApplyMovementSpeed();

	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				if (DefaultIMC) { Subsys->AddMappingContext(DefaultIMC, /*Priority*/0); }
			}
		}
	}

	// Ensure sprint speed state is applied on spawn
	GetCharacterMovement()->MaxWalkSpeed = bWantsToSprint ? SprintSpeed : WalkSpeed;
}

// Called to bind functionality to input
void ASFW_PlayerBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Axes (Triggered each frame)
		if (IA_Move) EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ASFW_PlayerBase::Move);
		if (IA_Look) EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &ASFW_PlayerBase::Look);

		// Sprint (press/hold)
		if (IA_Sprint)
		{
			EIC->BindAction(IA_Sprint, ETriggerEvent::Started, this, &ASFW_PlayerBase::SprintStarted);
			EIC->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &ASFW_PlayerBase::SprintCompleted);
			EIC->BindAction(IA_Sprint, ETriggerEvent::Canceled, this, &ASFW_PlayerBase::SprintCompleted);
		}

		// Crouch (hold to crouch)
		if (IA_Crouch)
		{
			EIC->BindAction(IA_Crouch, ETriggerEvent::Started, this, &ASFW_PlayerBase::CrouchStarted);
			EIC->BindAction(IA_Crouch, ETriggerEvent::Completed, this, &ASFW_PlayerBase::CrouchCompleted);
			EIC->BindAction(IA_Crouch, ETriggerEvent::Canceled, this, &ASFW_PlayerBase::CrouchCompleted);
		}

		if (IA_Interact)
		{
			EIC->BindAction(IA_Interact, ETriggerEvent::Started, this, &ASFW_PlayerBase::TryInteract);
		}

		if (IA_ToggleHeadLamp)
		{
			EIC->BindAction(IA_ToggleHeadLamp, ETriggerEvent::Started, this, &ASFW_PlayerBase::HandleToggleHeadLamp);
		}
	}
}

void ASFW_PlayerBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	UpdateMeshVisibility();
}

void ASFW_PlayerBase::OnRep_Controller()
{
	Super::OnRep_Controller();
	UpdateMeshVisibility();
}

void ASFW_PlayerBase::ApplyMovementSpeed()
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (!Move) return;

	if (Move->IsCrouching())
	{
		Move->MaxWalkSpeed = CrouchSpeed;
		return;
	}

	Move->MaxWalkSpeed = bWantsToSprint ? SprintSpeed : WalkSpeed;
}

void ASFW_PlayerBase::OnRep_WantsToSprint()
{
	ApplyMovementSpeed();
}

void ASFW_PlayerBase::ServerSetSprint_Implementation(bool bSprint)
{
	bWantsToSprint = bSprint;
	ApplyMovementSpeed();
}

void ASFW_PlayerBase::Move(const FInputActionValue& Value)
{
	const FVector2D Axis2D = Value.Get<FVector2D>();
	if (!Controller || Axis2D.IsNearlyZero()) return;

	const FRotator YawRot(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FVector Fwd = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Rt = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

	AddMovementInput(Fwd, Axis2D.Y); // typically Y = forward on 2D stick
	AddMovementInput(Rt, Axis2D.X); // X = right
}

void ASFW_PlayerBase::Look(const FInputActionValue& Value)
{
	const FVector2D Axis2D = Value.Get<FVector2D>();
	AddControllerYawInput(Axis2D.X);
	AddControllerPitchInput(Axis2D.Y);
}

void ASFW_PlayerBase::SprintStarted(const FInputActionValue& Value)
{
	bWantsToSprint = true;
	ApplyMovementSpeed();               // local prediction
	if (!HasAuthority()) ServerSetSprint(true);

}

void ASFW_PlayerBase::SprintCompleted(const FInputActionValue& Value)
{
	bWantsToSprint = false;
	ApplyMovementSpeed();               // local prediction
	if (!HasAuthority()) ServerSetSprint(false);
}


void ASFW_PlayerBase::UpdateMeshVisibility()
{
	const bool bLocal = IsLocallyControlled();

	if (Mesh1P)
	{
		Mesh1P->SetVisibility(bLocal, true);
		Mesh1P->SetOnlyOwnerSee(true);
	}
	if (USkeletalMeshComponent* Mesh3P = GetMesh())
	{
		Mesh3P->SetOwnerNoSee(bLocal); //hide full body for local only
	}
	if (FirstPersonCamera)
	{
		FirstPersonCamera->SetActive(bLocal);
	}

}

void ASFW_PlayerBase::Server_Interact_Implementation(AActor* Target)
{
	if (!IsValid(Target)) return;
	// (optional) validate range here

	if (Target->GetClass()->ImplementsInterface(USFW_InteractableInterface::StaticClass()))
	{
		ISFW_InteractableInterface::Execute_Interact(Target, Controller); // runs on server
	}
}

void ASFW_PlayerBase::TryInteract()
{
	if (!Controller) return;

	FVector EyesLoc; FRotator EyesRot;
	Controller->GetPlayerViewPoint(EyesLoc, EyesRot);

	const FVector Start = EyesLoc;
	const FVector End = Start + EyesRot.Vector() * InteractRange;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(InteractTrace), false, this);

	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		AActor* HitActor = Hit.GetActor();                      //  declare it

		if (IsValid(HitActor) &&
			HitActor->GetClass()->ImplementsInterface(USFW_InteractableInterface::StaticClass()))
		{
			if (HasAuthority())
			{
				ISFW_InteractableInterface::Execute_Interact(HitActor, Controller);
			}
			else
			{
				Server_Interact(HitActor);                      // owning client -> server
			}
		}

		// Safe debug
		if (IsValid(HitActor))
		{
			UE_LOG(LogTemp, Warning, TEXT("Hit: %s"), *HitActor->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Hit: (no actor)"));
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("TryInteract pressed"));
	DrawDebugLine(GetWorld(), Start, End, FColor::Cyan, false, 1.5f, 0, 1.f);
}


void ASFW_PlayerBase::HandleToggleHeadLamp(const FInputActionValue& Value)
{
	if (USFW_EquipmentManagerComponent* Mgr = FindComponentByClass<USFW_EquipmentManagerComponent>())
	{
		Mgr->Server_ToggleHeadLamp();
	}
}


void ASFW_PlayerBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASFW_PlayerBase, bWantsToSprint);
}