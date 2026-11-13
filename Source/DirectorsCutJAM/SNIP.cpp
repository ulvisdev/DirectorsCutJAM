#include "SNIP.h"
#include "SNIPPresentation.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Animation/AnimInstance.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Modules/ModuleManager.h"
#include "Sound/SoundBase.h"
IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, DirectorsCutJAM, "DirectorsCutJAM");
DEFINE_LOG_CATEGORY_STATIC(LogSNIP,Log,All);
static void SNIPSound(UObject* Context,const TCHAR* Name)
{
    const FString Path=FString::Printf(TEXT("/Game/SNIP/Audio/%s.%s"),Name,Name);
    float Volume=1;
    if(auto* P=Cast<ASNIPCharacter>(UGameplayStatics::GetPlayerCharacter(Context,0)))if(P->Presentation)Volume=P->Presentation->GetVolume();
    if(auto* Sound=LoadObject<USoundBase>(nullptr,*Path))UGameplayStatics::PlaySound2D(Context,Sound,.35f*Volume);
}
ASNIPPuzzle::ASNIPPuzzle()
{
    PrimaryActorTick.bCanEverTick=true;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("GridRoot"));
}
ASNIPCharacter* ASNIPPuzzle::Player() const
{
    return Cast<ASNIPCharacter>(UGameplayStatics::GetPlayerCharacter(this,0));
}
FVector ASNIPPuzzle::CellWorld(FIntVector P) const
{
    return GetActorLocation()+FVector(P)*Spacing+FVector(Spacing*.5f);
}
FString ASNIPPuzzle::Title() const
{
    const TCHAR* Titles[]={TEXT("01 / THE MISSING BRIDGE"),TEXT("02 / TOO HIGH!"),TEXT("03 / THINK DIFFERENTLY"),TEXT("04 / TRAPDOOR"),TEXT("05 / RIDING THE SEAM")};
    return Titles[FMath::Clamp(Stage,0,4)];
}
void ASNIPPuzzle::AddCell(FIntVector P,bool Metal)
{
    FSNIPCell C;
    C.Grid=P;
    C.Metal=Metal;
    auto* M=NewObject<UStaticMeshComponent>(this);
    AddInstanceComponent(M);
    M->SetupAttachment(RootComponent);
    M->SetMobility(EComponentMobility::Movable);
    M->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    auto* Chosen=Metal?MetalMaterial.Get():PaperMaterial.Get();
    M->SetMaterial(0,Chosen?Chosen:LoadObject<UMaterialInterface>(nullptr,Metal?TEXT("/Game/SNIP/Materials/M_Metal.M_Metal"):TEXT("/Game/SNIP/Materials/M_Paper.M_Paper")));
    if(!Metal)if(auto* Mat=M->CreateDynamicMaterialInstance(0))
    {
        // One deliberate palette for generated prototype paper; assigned final art is untouched.
        if(!Chosen || Chosen->GetPathName().StartsWith(TEXT("/Game/SNIP/Cinematic/")))
        {
            const FLinearColor Colors[]={FLinearColor(.949f,.894f,.784f),FLinearColor(.537f,.580f,.467f),FLinearColor(.337f,.553f,.576f),FLinearColor(.839f,.424f,.396f),FLinearColor(.949f,.894f,.784f)};
            Mat->SetVectorParameterValue(TEXT("Tint"),Colors[FMath::Clamp(Stage,0,4)]);
        }
    }
    M->SetCollisionProfileName(TEXT("BlockAll"));
    M->RegisterComponent();
    M->SetWorldLocation(CellWorld(P));
    M->SetWorldScale3D(FVector(Spacing/100.f));
    C.Mesh=M;
    Cells.Add(C);
}
void ASNIPPuzzle::BuildPuzzle()
{
    for(auto& C:Cells)if(C.Mesh)
    {
        TArray<USceneComponent*> AttachedParts;C.Mesh->GetChildrenComponents(true,AttachedParts);
        for(auto* Child:AttachedParts)Child->DestroyComponent();
        C.Mesh->DestroyComponent();
    }
    Cells.Empty();
    if(ExitMesh)ExitMesh->DestroyComponent();
    Stage=FMath::Clamp(Stage,0,4);
    Busy=false;
    Complete=false;
    FinishTime=0;
    Cursor=0;
    History.Empty();
    Dimensions=Stage==0?FIntVector(12,5,6):Stage==1?FIntVector(8,5,8):FIntVector(10,10,8);
    if(Stage==0)
    {
        for(int x=0; x<12; x++)for(int y=0; y<5; y++)if(x<4||x>=9)AddCell({x,y,0});
        SpawnPoint=CellWorld({1,2,0})+FVector(0,0,Spacing*.5f+90);
        ExitPosition=CellWorld({10,2,0})+FVector(0,0,Spacing*.5f+95);
    }
    if(Stage==1)
    {
        for(int x=0; x<8; x++)for(int y=0; y<5; y++)
        {
            if(x<4)AddCell({x,y,0});
            else
            {
                for(int z=0; z<4; z++)AddCell({x,y,z});
                AddCell({x,y,4},true);
            }
        }
        for(int y=0; y<5; y++)AddCell({3,y,1});
        SpawnPoint=CellWorld({1,2,0})+FVector(0,0,Spacing*.5f+90);
        ExitPosition=CellWorld({6,2,4})+FVector(0,0,Spacing*.5f+95);
    }
    if(Stage==2)
    {
        // An L-shaped route separated in X, Y and Z requires at least two
        // editing perspectives. Metal protects the islands from broad cuts.
        for(int x=0; x<3; x++)for(int y=0; y<3; y++)AddCell({x,y,0},y==0);
        for(int x=0; x<3; x++)for(int y=7; y<10; y++)AddCell({x,y,0});
        for(int y=7; y<10; y++)AddCell({2,y,1});
        for(int x=7; x<10; x++)for(int y=7; y<10; y++)
        {
            for(int z=0; z<4; z++)AddCell({x,y,z});
            AddCell({x,y,4},true);
        }
        SpawnPoint=CellWorld({1,1,0})+FVector(0,0,Spacing*.5f+90);
        ExitPosition=CellWorld({8,8,4})+FVector(0,0,Spacing*.5f+95);
    }
    if(Stage==3)
    {
        // Cut the paper floor you stand on; drop into a protected lower gallery.
        Dimensions=FIntVector(9,5,9);
        for(int x=0;x<9;x++)for(int y=0;y<5;y++)AddCell({x,y,0},true);
        for(int x=0;x<9;x++)for(int y=0;y<5;y++)AddCell({x,y,5});
        for(int x=0;x<9;x++)for(int y=0;y<5;y++)if(x==0||x==8||y==0||y==4)
            for(int z=1;z<5;z++)AddCell({x,y,z},true);
        // A metal partition cannot be cut. The upper deck is the only way over it.
        for(int y=0;y<5;y++)for(int z=1;z<5;z++)AddCell({5,y,z},true);
        SpawnPoint=CellWorld({1,2,5})+FVector(0,0,Spacing*.5f+90);
        ExitPosition=CellWorld({7,2,0})+FVector(0,0,Spacing*.5f+95);
    }
    if(Stage==4)
    {
        Dimensions=FIntVector(14,12,9);
        // Ride a surviving elevated paper island as empty space is removed.
        for(int x=0;x<3;x++)for(int y=0;y<3;y++)AddCell({x,y,0},true);
        for(int x=8;x<11;x++)for(int y=0;y<3;y++)AddCell({x,y,4});
        for(int x=8;x<11;x++)for(int y=9;y<12;y++)AddCell({x,y,0},true);
        SpawnPoint=CellWorld({9,1,4})+FVector(0,0,Spacing*.5f+90);
        ExitPosition=CellWorld({9,10,0})+FVector(0,0,Spacing*.5f+95);
    }
    if(Stage==0)for(auto& C:Cells)if(C.Grid==FIntVector(2,2,0))
    {
        auto* Prop=NewObject<UStaticMeshComponent>(this);
        AddInstanceComponent(Prop);Prop->SetupAttachment(C.Mesh);
        Prop->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
        Prop->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SNIP/Cinematic/Materials/M_SNIP_Cardboard.M_SNIP_Cardboard")));
        Prop->SetCollisionProfileName(TEXT("BlockAll"));Prop->SetMobility(EComponentMobility::Movable);
        Prop->SetRelativeLocation(FVector(0,0,65));Prop->SetRelativeScale3D(FVector(.45f,.9f,.3f));Prop->RegisterComponent();
        // Inherit deletion, restoration and motion with its owning paper cell.
    }
    ExitMesh=NewObject<UStaticMeshComponent>(this);
    AddInstanceComponent(ExitMesh);
    ExitMesh->SetupAttachment(RootComponent);
    ExitMesh->SetMobility(EComponentMobility::Movable);
    ExitMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    ExitMesh->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SNIP/Materials/M_Exit.M_Exit")));
    ExitMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ExitMesh->RegisterComponent();
    ExitMesh->SetWorldLocation(ExitPosition);
    ExitMesh->SetWorldScale3D(FVector(.45f,.8f,1.8f));
    if(auto* P=Player())
    {
        P->SetActorLocation(SpawnPoint,false,nullptr,ETeleportType::TeleportPhysics);
        P->FrozenVelocity=FVector::ZeroVector;
        P->MovementFrozen=false;
        P->GetCharacterMovement()->SetComponentTickEnabled(true);
        P->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        P->GetCharacterMovement()->StopMovementImmediately();
        P->Cancel();
        P->ApplyView(0);
    }
    Notice=TEXT("Reach the mint doorway. Cut away space to bring the world together.");
    InitialState=Capture();
}
void ASNIPPuzzle::BeginPlay()
{
    Super::BeginPlay();
    for(TActorIterator<AActor> It(GetWorld());It;++It)
        if(It->ActorHasTag(TEXT("SNIPDecoration")))It->SetActorEnableCollision(false);
    BuildPuzzle();
}
FSNIPState ASNIPPuzzle::Capture() const
{
    FSNIPState S;
    for(const auto& C:Cells)
    {
        S.Positions.Add(C.Grid);
        S.Alive.Add(C.Alive);
    }
    S.Size=Dimensions;
    S.Exit=ExitPosition;
    S.Player=Player()?Player()->GetActorLocation():SpawnPoint;
    return S;
}
FString ASNIPPuzzle::ValidateCut(int A,int L,int H) const
{
    if(Busy)return TEXT("Let the paper settle.");
    if(Complete)return TEXT("Puzzle complete.");
    if(A<0||A>2||L<0||H>Dimensions[A]||L>=H||H-L>=Dimensions[A])return TEXT("Select a nonempty interval inside the grid.");
    const float Min=GetActorLocation()[A]+L*Spacing,Max=GetActorLocation()[A]+H*Spacing;
    auto Hit=[&](float V,float Radius)
    {
        return V+Radius>Min+.1f && V-Radius<Max-.1f;
    };
    if(Hit(ExitPosition[A],A==2?90:45))return TEXT("The exit is protected.");
    for(const auto& C:Cells)if(C.Alive&&C.Metal&&C.Grid[A]>=L&&C.Grid[A]<H)return TEXT("Metal cannot be snipped. Choose another layer.");
    // Keep a surviving landing under the performer and doorway. Slabs may remove
    // a support layer only when compression supplies another surface below it.
    auto HasLanding=[&](FVector Q)
    {
        if(Q[A]>=Max)Q[A]-=(H-L)*Spacing;
        for(const auto& C:Cells)if(C.Alive&&!(C.Grid[A]>=L&&C.Grid[A]<H))
        {
            FIntVector G=C.Grid;
            if(G[A]>=H)G[A]-=H-L;
            FVector Center=CellWorld(G);
            if(FMath::Abs(Q.X-Center.X)<Spacing*.5f&&FMath::Abs(Q.Y-Center.Y)<Spacing*.5f&&Center.Z+Spacing*.5f<=Q.Z)return true;
        }
        return false;
    };
    if(!HasLanding(ExitPosition))return TEXT("Leave a landing beneath the exit.");
    return TEXT("");
}
bool ASNIPPuzzle::ApplyCut(int A,int L,int H)
{
    FString Why=ValidateCut(A,L,H);
    if(!Why.IsEmpty())
    {
        Notice=Why;
        if(auto* P=Player())P->Presentation->CutResult(false,Why);
        SNIPSound(this,TEXT("SNIP_Reject"));
        return false;
    }
    if(auto* P=Player())if(P->View==0)
    {
        Notice=TEXT("Choose an orthographic view first.");
        return false;
    }
    if(auto* P=Player())if(P->Presentation->BlocksEditing())return false;
    History.SetNum(Cursor);
    FSNIPCommand Command;
    Command.Before=Capture();
    FSNIPState S=Command.Before;
    const int D=H-L;
    for(int i=0; i<Cells.Num(); i++)if(S.Alive[i])
    {
        if(S.Positions[i][A]>=L&&S.Positions[i][A]<H)S.Alive[i]=false;
        else if(S.Positions[i][A]>=H)S.Positions[i][A]-=D;
    }
    const float End=GetActorLocation()[A]+H*Spacing;
    S.Player[A]=CompressCoordinate(S.Player[A],End-D*Spacing,End);
    // Use the exact surviving support displacement, including a capsule straddling a seam.
    if(auto* P=Player())for(int i=0;i<Cells.Num();++i)
    {
        const FVector Center=CellWorld(Cells[i].Grid),Q=P->GetActorLocation();
        if(Cells[i].Alive && S.Alive[i] && FMath::Abs(Q.Z-88-(Center.Z+Spacing*.5f))<8 &&
           FMath::Abs(Q.X-Center.X)<Spacing*.5f && FMath::Abs(Q.Y-Center.Y)<Spacing*.5f)
            S.Player=Q+CellWorld(S.Positions[i])-Center;
    }
    Seam=GetActorLocation()+FVector(Dimensions)*Spacing*.5f;
    Seam[A]=End-D*Spacing;
    if(S.Exit[A]>=End)S.Exit[A]-=D*Spacing;
    S.Size[A]-=D;
    Command.After=S;
    History.Add(Command);
    ++Cursor;
    Restore(S);
    if(auto* P=Player())P->Presentation->CutResult(true,TEXT(""));
    SNIPSound(this,TEXT("SNIP_Snip"));
    SNIPSound(this,TEXT("SNIP_Slide"));
    Notice=TEXT("SNIP!  Space removed. Possibilities added.");
    UE_LOG(LogSNIP,Display,TEXT("Cut axis=%d [%d,%d) history=%d"),A,L,H,Cursor);
    return true;
}
void ASNIPPuzzle::Restore(const FSNIPState& S,int32 Style)
{
    From.Empty();
    To.Empty();
    for(int i=0; i<Cells.Num(); i++)
    {
        auto& C=Cells[i];
        From.Add(C.Mesh->GetComponentLocation());
        C.Grid=S.Positions[i];
        C.Alive=S.Alive[i];
        To.Add(CellWorld(C.Grid));
        C.Mesh->SetVisibility(C.Alive,true);
        C.Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        TArray<USceneComponent*> AttachedParts;C.Mesh->GetChildrenComponents(true,AttachedParts);
        for(auto* Child:AttachedParts)if(auto* Mesh=Cast<UStaticMeshComponent>(Child))Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    PlayerFrom=Player()?Player()->GetActorLocation():S.Player;
    PlayerTo=SafePlayerPosition(S.Player);
    ExitFrom=ExitPosition;
    ExitPosition=S.Exit;
    Dimensions=S.Size;
    if(auto* P=Player())
    {
        P->FreezeMovement();
        P->FrozenMode=MOVE_Falling; // Edited support must be reacquired; pure view changes preserve the mode.
        P->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        P->Cancel();
    }
    Animation=0;
    AnimationStyle=Style;
    ImpactPlayed=false;
    Busy=true;
    Complete=false;
}
void ASNIPPuzzle::Undo()
{
    if(!Busy&&Cursor>0)
    {
        FSNIPState S=History[--Cursor].Before;
        if(auto* P=Player())S.Player=P->GetActorLocation();
        Restore(S,1);
        if(auto* P=Player())P->Presentation->HistoryUsed(true);
        SNIPSound(this,TEXT("SNIP_Undo"));
        Notice=TEXT("Unsnipped. Try a different idea.");
    }
}
void ASNIPPuzzle::Redo()
{
    if(!Busy&&Cursor<History.Num())
    {
        FSNIPState S=History[Cursor++].After;
        if(auto* P=Player())S.Player=P->GetActorLocation();
        Restore(S,2);
        if(auto* P=Player())P->Presentation->HistoryUsed(false);
        SNIPSound(this,TEXT("SNIP_Redo"));
        Notice=TEXT("Snipped again.");
    }
}
void ASNIPPuzzle::Restart()
{
    if(!Busy)if(auto* P=Player())P->Presentation->Retry();
}
void ASNIPPuzzle::Tick(float Dt)
{
    Super::Tick(Dt);
    ImpactPulse=FMath::Max(0.f,ImpactPulse-Dt*8);
    if(Busy)
    {
        Animation+=Dt;
        const float Scale=AnimationStyle==2?.65f:1.f;
        const float Snip=FMath::Max(.001f,SnipSeconds)*Scale;
        const float Ant=FMath::Max(.001f,AnticipationSeconds)*Scale;
        const float Slide=FMath::Max(.01f,CompressionSeconds)*Scale;
        const float Impact=FMath::Max(.001f,ImpactSeconds)*Scale;
        const float HitTime=Snip+Ant+Slide;
        const float Duration=AnimationStyle==1?.2f:HitTime+Impact;
        const float T=FMath::Clamp(Animation/Duration,0.f,1.f);
        float E=AnimationStyle==1?1-FMath::Pow(1-T,3):
            FMath::Pow(FMath::Clamp((Animation-Snip-Ant)/Slide,0.f,1.f),FMath::Max(1.f,CompressionExponent));
        float Pull=0;
        if(AnimationStyle!=1 && Animation>=Snip && Animation<HitTime)
            Pull=Spacing*AnticipationTiles*(Animation<Snip+Ant?(Animation-Snip)/Ant:1-E);
        float Shake=0;
        if(AnimationStyle!=1 && Animation>=HitTime)
        {
            if(!ImpactPlayed){ImpactPlayed=true;ImpactPulse=1.f;SNIPSound(this,TEXT("SNIP_Impact"));}
            Shake=FMath::Sin((Animation-HitTime)*180)*VibrationAmplitude*(1-FMath::Clamp((Animation-HitTime)/Impact,0.f,1.f));
        }
        for(int i=0; i<Cells.Num(); i++)
        {
            const FVector Direction=(From[i]-To[i]).GetSafeNormal();
            Cells[i].Mesh->SetWorldLocation(FMath::Lerp(From[i],To[i],E)+Direction*(Pull+Shake));
        }
        ExitMesh->SetWorldLocation(FMath::Lerp(ExitFrom,ExitPosition,E));
        if(auto* P=Player())P->SetActorLocation(FMath::Lerp(PlayerFrom,PlayerTo,E),false,nullptr,ETeleportType::TeleportPhysics);
        if(T>=1)
        {
            Busy=false;
            for(int i=0;i<Cells.Num();++i)Cells[i].Mesh->SetWorldLocation(To[i]);
            for(auto& C:Cells)
            {
                auto Collision=C.Alive?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision;
                C.Mesh->SetCollisionEnabled(Collision);
                TArray<USceneComponent*> AttachedParts;C.Mesh->GetChildrenComponents(true,AttachedParts);
                for(auto* Child:AttachedParts)if(auto* Mesh=Cast<UStaticMeshComponent>(Child))Mesh->SetCollisionEnabled(Collision);
            }
            if(auto* P=Player())
            {
                P->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
                P->SetActorLocation(PlayerTo,false,nullptr,ETeleportType::TeleportPhysics);
                if(!P->View)P->ResumeMovement();
            }
        }
        return;
    }
    auto* P=Player();
    if(!P||P->Presentation->BlocksMovement())return;
    if(!P->View&&!Complete&&FVector::Dist(P->GetActorLocation(),ExitPosition)<130)
    {
        Complete=true;
        P->Presentation->NextScene();
        return;
    }
    if(!P->View&&P->GetActorLocation().Z<GetActorLocation().Z-550)
        P->Presentation->Retry(true);

}
ASNIPCharacter::ASNIPCharacter()
{
    PrimaryActorTick.bCanEverTick=true;
    Presentation=CreateDefaultSubobject<USNIPPresentation>(TEXT("FilmPresentation"));
    GetCapsuleComponent()->InitCapsuleSize(34,88);
    bUseControllerRotationYaw=false;
    auto* M=GetCharacterMovement();
    M->MaxWalkSpeed=430;
    M->MaxAcceleration=2600;
    M->BrakingDecelerationWalking=2500;
    M->JumpZVelocity=880;
    M->GravityScale=1.5f;
    M->AirControl=.6f;
    M->bOrientRotationToMovement=true;
    M->RotationRate=FRotator(0,720,0);
    M->MaxSimulationIterations=32;
    GetMesh()->SetSkeletalMesh(LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple")));
    GetMesh()->SetRelativeLocation(FVector(0,0,-88));
    GetMesh()->SetRelativeRotation(FRotator(0,-90,0));
    GetMesh()->SetAnimInstanceClass(LoadClass<UAnimInstance>(nullptr,TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C")));
    for(int i=0; i<GetMesh()->GetNumMaterials(); i++)GetMesh()->SetMaterial(i,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SNIP/Materials/M_Coral.M_Coral")));
    ViewCamera=CreateDefaultSubobject<UCameraComponent>(TEXT("DioramaCamera"));
    ViewCamera->SetupAttachment(RootComponent);
    ViewCamera->SetAbsolute(true,true,true);
    ViewCamera->FieldOfView=48;
    ViewCamera->OrthoWidth=3200;
    // Auto plane correction recentres an orthographic camera on its owning
    // pawn. Fixed planes preserve our whole-diorama framing and deprojection.
    ViewCamera->bAutoCalculateOrthoPlanes=false;
    ViewCamera->bUpdateOrthoPlanes=false;
    ViewCamera->bUseCameraHeightAsViewTarget=false;
    ViewCamera->OrthoNearClipPlane=1;
    ViewCamera->OrthoFarClipPlane=20000;
    ViewCamera->bOverrideAspectRatioAxisConstraint=true;
    ViewCamera->AspectRatioAxisConstraint=AspectRatio_MaintainXFOV;
}
void ASNIPCharacter::BeginPlay()
{
    Super::BeginPlay();
    for(TActorIterator<ASNIPPuzzle>I(GetWorld()); I; ++I)
    {
        Puzzle=*I;
        break;
    }
    ApplyView(0);
    Presentation->Initialize();
}
bool ASNIPCharacter::CanMove() const
{
    return !View&&Puzzle&&!Puzzle->Busy&&!Puzzle->Complete&&!Presentation->BlocksMovement();
}
void ASNIPCharacter::MoveForward(float V)
{
    if(CanMove())AddMovementInput(FRotator(0,-45,0).Vector(),V);
}
void ASNIPCharacter::MoveRight(float V)
{
    if(CanMove())AddMovementInput(FRotationMatrix(FRotator(0,-45,0)).GetUnitAxis(EAxis::Y),V);
}
void ASNIPCharacter::JumpPressed()
{
    if(CanMove())Jump();
}
void ASNIPCharacter::SetView(int32 Mode)
{
    if(Presentation&&!Presentation->Applying){Presentation->StartView(Mode);return;}
    ApplyView(Mode);
}
void ASNIPCharacter::ApplyView(int32 Mode)
{
    if(Puzzle&&Puzzle->Busy)return;
    const int Next=FMath::Clamp(Mode,0,3);
    if(Next && !View)FreezeMovement();
    View=Next;
    Cancel();
    if(!View&&!Presentation->BlocksMovement())ResumeMovement();
    if(auto* PC=Cast<APlayerController>(Controller))
    {
        PC->bShowMouseCursor=View!=0;
        if(View)PC->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
        else PC->SetInputMode(FInputModeGameOnly());
    }
}
void ASNIPCharacter::SetupPlayerInputComponent(UInputComponent* I)
{
    Super::SetupPlayerInputComponent(I);
    I->BindAxis("SNIPForward",this,&ASNIPCharacter::MoveForward);
    I->BindAxis("SNIPRight",this,&ASNIPCharacter::MoveRight);
    I->BindAction("SNIPJump",IE_Pressed,this,&ASNIPCharacter::JumpPressed);
    I->BindAction("SNIPJump",IE_Released,this,&ACharacter::StopJumping);
    I->BindAction("SNIPGame",IE_Pressed,this,&ASNIPCharacter::GameView);
    I->BindAction("SNIPFront",IE_Pressed,this,&ASNIPCharacter::Front);
    I->BindAction("SNIPTop",IE_Pressed,this,&ASNIPCharacter::Top);
    I->BindAction("SNIPSide",IE_Pressed,this,&ASNIPCharacter::Side);
    I->BindAction("SNIPSelect",IE_Pressed,this,&ASNIPCharacter::MouseDown);
    I->BindAction("SNIPSelect",IE_Released,this,&ASNIPCharacter::MouseUp);
    I->BindAction("SNIPCancel",IE_Pressed,this,&ASNIPCharacter::Cancel);
    I->BindAction("SNIPUndo",IE_Pressed,this,&ASNIPCharacter::Undo);
    I->BindAction("SNIPRedo",IE_Pressed,this,&ASNIPCharacter::Redo);
    I->BindAction("SNIPRestart",IE_Pressed,this,&ASNIPCharacter::Restart);
    I->BindAction("SNIPMenu",IE_Pressed,Presentation.Get(),&USNIPPresentation::ToggleMenu);
    I->BindAction("SNIPSkip",IE_Pressed,Presentation.Get(),&USNIPPresentation::SkipTutorial);
    I->BindKey(EKeys::Up,IE_Pressed,Presentation.Get(),&USNIPPresentation::Up);
    I->BindKey(EKeys::Down,IE_Pressed,Presentation.Get(),&USNIPPresentation::Down);
    I->BindKey(EKeys::Enter,IE_Pressed,Presentation.Get(),&USNIPPresentation::Accept);
}
FVector ASNIPCharacter::MouseWorld() const
{
    FVector O=FVector::ZeroVector,D=FVector::ZeroVector;
    if(auto* PC=Cast<APlayerController>(Controller))PC->DeprojectMousePositionToWorld(O,D);
    if(Puzzle)
    {
        const int Hidden=View==1?1:View==2?2:0;
        const float Plane=Puzzle->GetActorLocation()[Hidden]+Puzzle->Dimensions[Hidden]*Puzzle->Spacing*.5f;
        if(FMath::Abs(D[Hidden])>.001f)O+=D*((Plane-O[Hidden])/D[Hidden]);
    }
    return O;
}
void ASNIPCharacter::MouseDown()
{
    if(Presentation->MenuOpen){Presentation->Click();return;}
    if(Presentation->BlocksEditing()||!View||!Puzzle||Puzzle->Busy||Puzzle->Complete)return;
    auto* PC=Cast<APlayerController>(Controller);
    if(!PC || !PC->GetMousePosition(DragScreen.X,DragScreen.Y))return;
    DragWorld=MouseWorld();
    Selecting=true;
    AxisLocked=false;
    Low=High=0;
    Rejected=false;
}
void ASNIPCharacter::MouseUp()
{
    if(!Selecting)return;
    UpdateSelection();
    const bool Ready=AxisLocked&&Low<High;
    Selecting=false;
    if(Ready)
    {
        Rejected=!Puzzle->ApplyCut(Axis,Low,High);
        ScissorFeedback=1;
    }
    Cancel();
}

void ASNIPCharacter::Cancel()
{
    Selecting=false;
    Locked=false;
    AxisLocked=false;
}
void ASNIPCharacter::Undo()
{
    if(Puzzle&&!Presentation->BlocksEditing())Puzzle->Undo();
}
void ASNIPCharacter::Redo()
{
    if(Puzzle&&!Presentation->BlocksEditing())Puzzle->Redo();
}
void ASNIPCharacter::Restart()
{
    if(Puzzle&&!Presentation->BlocksEditing())Puzzle->Restart();
}
void ASNIPCharacter::Tick(float Dt)
{
    Super::Tick(Dt);
    Presentation->Update(Dt);
    if(!Puzzle)return;
    FVector Center=Puzzle->GetActorLocation()+FVector(Puzzle->Dimensions)*Puzzle->Spacing*.5f;
    if(!View)Center.Z=Puzzle->GetActorLocation().Z+(Puzzle->Stage==0?250:450);
    FRotator Rot=View==0?FRotator(-38,-45,0):View==1?FRotator(0,90,0):View==2?FRotator(-90,-90,0):FRotator(0,180,0);
    ViewCamera->SetProjectionMode(View?ECameraProjectionMode::Orthographic:ECameraProjectionMode::Perspective);
    int W=1600,H=900;
    if(auto* PC=Cast<APlayerController>(Controller))PC->GetViewportSize(W,H);
    const float Aspect=float(W)/FMath::Max(1,H);
    const float Width=(View==3?Puzzle->Dimensions.Y:Puzzle->Dimensions.X)*Puzzle->Spacing;
    const float Height=(View==2?Puzzle->Dimensions.Y:Puzzle->Dimensions.Z)*Puzzle->Spacing;
    ViewCamera->SetOrthoWidth(FMath::Max(Width+550,(Height+550)*Aspect));
    FVector Target=Center-Rot.Vector()*(View?4200:5000);
    const bool Snap=Presentation->SnapCamera;
    ViewCamera->SetWorldLocation(Snap?Target:FMath::VInterpTo(ViewCamera->GetComponentLocation(),Target,Dt,12));
    ViewCamera->SetWorldRotation(Snap?Rot:FMath::RInterpTo(ViewCamera->GetComponentRotation(),Rot,Dt,14));
    Presentation->SnapCamera=false;
    ScissorFeedback=FMath::Max(0.f,ScissorFeedback-Dt*6);
    if(Puzzle->ImpactPulse>0)
        ViewCamera->AddWorldOffset(ViewCamera->GetRightVector()*FMath::Sin(GetWorld()->GetTimeSeconds()*120)*Puzzle->CameraShakeIntensity*Puzzle->ImpactPulse);
    ViewCamera->PostProcessSettings.bOverride_ColorSaturation=true;
    ViewCamera->PostProcessSettings.ColorSaturation=FVector4(View?.82f:1.f,View?.86f:1.f,View?.92f:1.f,1.f);
    if(Selecting)UpdateSelection();
}
void ASNIPCharacter::UpdateSelection()
{
    auto* PC=Cast<APlayerController>(Controller);
    if(!PC||!Puzzle||!Selecting)return;
    FVector2D Mouse;
    if(!PC->GetMousePosition(Mouse.X,Mouse.Y))return;
    const FVector2D Delta=Mouse-DragScreen;
    const float X=FMath::Abs(Delta.X),Y=FMath::Abs(Delta.Y);
    if(Delta.Size()<DragThreshold){Low=High=0;return;}
    const int HA=View==3?1:0,VA=View==2?1:2;
    if(!AxisLocked)Axis=X>Y?HA:VA;
    else if(Axis==HA && Y>X*DirectionHysteresis)Axis=VA;
    else if(Axis==VA && X>Y*DirectionHysteresis)Axis=HA;
    AxisLocked=true;
    const FVector End=MouseWorld();
    const int StartTile=FMath::FloorToInt((DragWorld[Axis]-Puzzle->GetActorLocation()[Axis])/Puzzle->Spacing);
    const int EndTile=FMath::FloorToInt((End[Axis]-Puzzle->GetActorLocation()[Axis])/Puzzle->Spacing);
    Low=FMath::Min(StartTile,EndTile);
    High=FMath::Max(StartTile,EndTile)+1;
}
void ASNIPCharacter::FreezeMovement()
{
    if(MovementFrozen)return;
    auto* M=GetCharacterMovement();
    FrozenVelocity=M->Velocity;
    FrozenMode=M->MovementMode;FrozenCustomMode=M->CustomMovementMode;
    MovementFrozen=true;
    M->SetComponentTickEnabled(false);
    M->Velocity=FVector::ZeroVector;
}
void ASNIPCharacter::ResumeMovement()
{
    if(!MovementFrozen)return;
    auto* M=GetCharacterMovement();
    M->SetMovementMode(FrozenMode,FrozenCustomMode);
    M->Velocity=FrozenVelocity;
    M->SetComponentTickEnabled(true);
    MovementFrozen=false;
}
float ASNIPPuzzle::CompressCoordinate(float Value,float A,float B)
{
    return Value<=A?Value:Value>=B?Value-(B-A):A;
}
FVector ASNIPPuzzle::SafePlayerPosition(FVector Desired) const
{
    // Find the closest non-overlapping candidate on cell faces. Conservative capsule
    // bounds leave clearance and are deterministic, even while collision is disabled.
    TArray<FBox> Obstacles;
    for(const auto& C:Cells)if(C.Alive)
    {
        const FVector Center=CellWorld(C.Grid),Half(Spacing*.5f);
        Obstacles.Add(FBox(Center-Half,Center+Half));
        TArray<USceneComponent*> AttachedParts;C.Mesh->GetChildrenComponents(true,AttachedParts);
        for(auto* Part:AttachedParts)if(auto* Mesh=Cast<UStaticMeshComponent>(Part))
        {
            const FVector At=Center+Mesh->GetComponentLocation()-C.Mesh->GetComponentLocation();
            const FVector Extent=Mesh->CalcBounds(Mesh->GetComponentTransform()).BoxExtent;
            Obstacles.Add(FBox(At-Extent,At+Extent));
        }
    }
    auto Clear=[&](const FVector& Q)
    {
        for(const auto& Box:Obstacles)
        {
            const FVector D=(Q-Box.GetCenter()).GetAbs(),Extent=Box.GetExtent()+FVector(34,34,88);
            if(D.X<Extent.X && D.Y<Extent.Y && D.Z<Extent.Z)return false;
        }
        return true;
    };
    if(Clear(Desired))return Desired;
    FVector Best=Desired;
    float Distance=TNumericLimits<float>::Max();
    for(const auto& Box:Obstacles)for(int Axis=0;Axis<3;++Axis)for(float Sign:{1.f,-1.f})
    {
        FVector Candidate=Desired;
        Candidate[Axis]=Box.GetCenter()[Axis]+Sign*(Box.GetExtent()[Axis]+(Axis==2?90:36));
        const float D=FVector::DistSquared(Candidate,Desired);
        if(D<Distance && Clear(Candidate)){Best=Candidate;Distance=D;}
    }
    return Best;
}
void ASNIPHUD::DrawHUD()
{
    Super::DrawHUD();
    auto* P=Cast<ASNIPCharacter>(GetOwningPawn());
    if(!P||!P->Puzzle||!Canvas)return;
    auto* G=P->Puzzle.Get();
    float W=Canvas->SizeX,H=Canvas->SizeY,S=FMath::Clamp(W/1400.f,.8f,1.5f);
    FLinearColor Ink(.12f,.18f,.2f),Mint(.15f,.8f,.6f),Coral(.9f,.22f,.23f);
    auto Text=[&](FString T,float X,float Y,float Size,FLinearColor C)
    {
        DrawText(T,C,X,Y,GEngine->GetLargeFont(),Size*S*2);
    };
    auto Project=[&](FVector V)
    {
        FVector2D O;
        GetOwningPlayerController()->ProjectWorldLocationToScreen(V,O);
        return O;
    };
    if(P->View&&!P->Presentation->MenuOpen&&!P->Presentation->TransitionActive)
    {
        const int HA=P->View==3?1:0,VA=P->View==2?1:2;
        // A fine graph-paper grid makes selectable intervals visible even in empty space.
        for(int A:{HA,VA})for(int K=0; K<=G->Dimensions[A]; K++)
        {
            FVector B=G->GetActorLocation(),E=B;
            B[A]+=K*G->Spacing;
            E=B;
            E[A==HA?VA:HA]+=G->Dimensions[A==HA?VA:HA]*G->Spacing;
            auto B2=Project(B),E2=Project(E);
            DrawLine(B2.X,B2.Y,E2.X,E2.Y,FLinearColor(.3f,.4f,.4f,.22f),1);
        }
        if((P->Selecting||P->Locked)&&P->Low<P->High)
        {
            FString Why=G->ValidateCut(P->Axis,P->Low,P->High);
            FLinearColor C=Why.IsEmpty()?FLinearColor(.851f,.471f,.259f):Coral;
            FVector B=G->GetActorLocation(),E=B;
            B[P->Axis]+=P->Low*G->Spacing;
            E[P->Axis]+=P->High*G->Spacing;
            int Other=P->Axis==HA?VA:HA;
            E[Other]+=G->Dimensions[Other]*G->Spacing;
            auto Q=Project(B),R=Project(E);
            float X=FMath::Min(Q.X,R.X),Y=FMath::Min(Q.Y,R.Y),RW=FMath::Abs(Q.X-R.X),RH=FMath::Abs(Q.Y-R.Y);
            DrawRect(FLinearColor(C.R,C.G,C.B,.18f+.05f*FMath::Sin(GetWorld()->GetTimeSeconds()*8)),X,Y,RW,RH);
            for(float n=0; n<RW; n+=20)
            {
                DrawLine(X+n,Y,X+FMath::Min(n+10,RW),Y,C,2);
                DrawLine(X+n,Y+RH,X+FMath::Min(n+10,RW),Y+RH,C,2);
            }
            for(float n=0; n<RH; n+=20)
            {
                DrawLine(X,Y+n,X,Y+FMath::Min(n+10,RH),C,2);
                DrawLine(X+RW,Y+n,X+RW,Y+FMath::Min(n+10,RH),C,2);
            }

            TSet<FIntPoint> GhostTiles;
            if(Why.IsEmpty())for(const auto& Cell:G->Cells)if(Cell.Alive && Cell.Grid[P->Axis]>=P->High)
            {
                FVector Center=G->CellWorld(Cell.Grid);
                Center[P->Axis]=ASNIPPuzzle::CompressCoordinate(Center[P->Axis],G->GetActorLocation()[P->Axis]+P->Low*G->Spacing,G->GetActorLocation()[P->Axis]+P->High*G->Spacing);
                const FIntPoint Tile(FMath::RoundToInt(Center[HA]/G->Spacing),FMath::RoundToInt(Center[VA]/G->Spacing));
                if(GhostTiles.Contains(Tile))continue;
                GhostTiles.Add(Tile);
                FVector GhostMin=Center,GhostMax=Center;
                GhostMin[HA]-=G->Spacing*.48f; GhostMin[VA]-=G->Spacing*.48f;
                GhostMax[HA]+=G->Spacing*.48f; GhostMax[VA]+=G->Spacing*.48f;
                const auto U=Project(GhostMin),V=Project(GhostMax);
                FLinearColor Ghost(.1f,.72f,.85f,.9f);
                DrawRect(FLinearColor(.10f,.65f,.80f,.50f),FMath::Min(U.X,V.X),FMath::Min(U.Y,V.Y),FMath::Abs(U.X-V.X),FMath::Abs(U.Y-V.Y));
                DrawLine(U.X,U.Y,V.X,U.Y,Ghost,1);DrawLine(V.X,U.Y,V.X,V.Y,Ghost,1);
                DrawLine(V.X,V.Y,U.X,V.Y,Ghost,1);DrawLine(U.X,V.Y,U.X,U.Y,Ghost,1);
            }
        }
        float MX=0,MY=0;
        GetOwningPlayerController()->GetMousePosition(MX,MY);
        float Angle=P->Axis==VA?PI*.5f:0;
        auto V=[&](float x,float y)
        {
            return FVector2D(MX+FMath::Cos(Angle)*x-FMath::Sin(Angle)*y,MY+FMath::Sin(Angle)*x+FMath::Cos(Angle)*y);
        };
        float Spread=P->Selecting?20:FMath::Lerp(10.f,1.f,P->ScissorFeedback);
        if(P->Rejected)MX+=FMath::Sin(GetWorld()->GetTimeSeconds()*90)*6*P->ScissorFeedback;
        for(float Side:{-1.f,1.f})
        {
            auto A=V(5,Side*3),B=V(40,Side*Spread);
            DrawLine(A.X,A.Y,B.X,B.Y,Ink,4);
            auto C=V(-12,Side*12);
            for(int i=0; i<20; i++)
            {
                float a=i*2*PI/20,b=(i+1)*2*PI/20;
                DrawLine(C.X+9*FMath::Cos(a),C.Y+9*FMath::Sin(a),C.X+9*FMath::Cos(b),C.Y+9*FMath::Sin(b),Coral,3);
            }
        }
    }
    auto Exit=Project(G->ExitPosition+FVector(0,0,160));
    if(G->ImpactPulse>0)
    {
        const auto At=Project(G->Seam);
        const float Age=1-G->ImpactPulse;
        for(int i=0;i<FMath::Clamp(G->ImpactParticleCount,0,64);++i)
        {
            const float Angle=i*2.39996f;
            DrawRect(FLinearColor(.97f,.8f,.5f,G->ImpactPulse),At.X+FMath::Cos(Angle)*Age*95,At.Y+FMath::Sin(Angle)*Age*55+Age*Age*25,4,2);
        }
    }
    P->Presentation->Draw(this,Canvas);
}
ASNIPGameMode::ASNIPGameMode()
{
    DefaultPawnClass=ASNIPCharacter::StaticClass();
    HUDClass=ASNIPHUD::StaticClass();
}
