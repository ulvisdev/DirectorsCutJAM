#include "SNIP.h"
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
IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, DirectorsCutJAM, "DirectorsCutJAM");
DEFINE_LOG_CATEGORY_STATIC(LogSNIP,Log,All);
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
    return Stage==0?TEXT("01 / THE MISSING BRIDGE"):Stage==1?TEXT("02 / TOO HIGH!"):TEXT("03 / THINK DIFFERENTLY");
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
    M->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,Metal?TEXT("/Game/SNIP/Materials/M_Metal.M_Metal"):TEXT("/Game/SNIP/Materials/M_Paper.M_Paper")));
    if(!Metal)if(auto* Mat=M->CreateDynamicMaterialInstance(0))
    {
        const FLinearColor Colors[]={FLinearColor(.63f,.79f,.65f),FLinearColor(.59f,.75f,.84f),FLinearColor(.91f,.67f,.53f)};
        Mat->SetVectorParameterValue(TEXT("Tint"),Colors[(Stage+(P.X/4))%3]);
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
    for(auto& C:Cells)if(C.Mesh)C.Mesh->DestroyComponent();
    Cells.Empty();
    if(ExitMesh)ExitMesh->DestroyComponent();
    Stage=FMath::Clamp(Stage,0,2);
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
        P->GetCharacterMovement()->StopMovementImmediately();
        P->Cancel();
        P->SetView(0);
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
    if(auto* P=Player())
    {
        if(Hit(P->GetActorLocation()[A],A==2?88:34))return TEXT("Your paper performer is inside this cut.");
    }
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
    if(auto* P=Player())if(!HasLanding(P->GetActorLocation()))return TEXT("Leave some paper beneath your performer.");
    // Validate the final capsule against every surviving cell before accepting a cut.
    if(auto* P=Player())
    {
        FVector Q=P->GetActorLocation();
        if(Q[A]>=Max)Q[A]-=(H-L)*Spacing;
        for(const auto& C:Cells)if(C.Alive && !(C.Grid[A]>=L&&C.Grid[A]<H))
        {
            FIntVector G=C.Grid;
            if(G[A]>=H)G[A]-=H-L;
            FVector D=(Q-CellWorld(G)).GetAbs();
            if(D.X<Spacing*.5f+35 && D.Y<Spacing*.5f+35 && D.Z<Spacing*.5f+88.5f)return TEXT("That cut would squeeze the performer.");
        }
    }
    return TEXT("");
}
bool ASNIPPuzzle::ApplyCut(int A,int L,int H)
{
    FString Why=ValidateCut(A,L,H);
    if(!Why.IsEmpty())
    {
        Notice=Why;
        return false;
    }
    if(auto* P=Player())if(P->View==0)
    {
        Notice=TEXT("Choose an orthographic view first.");
        return false;
    }
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
    if(S.Player[A]>=End)S.Player[A]-=D*Spacing;
    if(S.Exit[A]>=End)S.Exit[A]-=D*Spacing;
    S.Size[A]-=D;
    Command.After=S;
    History.Add(Command);
    ++Cursor;
    Restore(S);
    Notice=TEXT("SNIP!  Space removed. Possibilities added.");
    UE_LOG(LogSNIP,Display,TEXT("Cut axis=%d [%d,%d) history=%d"),A,L,H,Cursor);
    return true;
}
void ASNIPPuzzle::Restore(const FSNIPState& S)
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
        C.Mesh->SetVisibility(C.Alive);
        C.Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    PlayerFrom=Player()?Player()->GetActorLocation():S.Player;
    PlayerTo=S.Player;
    ExitFrom=ExitPosition;
    ExitPosition=S.Exit;
    Dimensions=S.Size;
    if(auto* P=Player())
    {
        P->GetCharacterMovement()->StopMovementImmediately();
        P->GetCharacterMovement()->DisableMovement();
        P->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        P->Cancel();
    }
    Animation=0;
    Busy=true;
    Complete=false;
}
void ASNIPPuzzle::Undo()
{
    if(!Busy&&Cursor>0)
    {
        Restore(History[--Cursor].Before);
        Notice=TEXT("Unsnipped. Try a different idea.");
    }
}
void ASNIPPuzzle::Redo()
{
    if(!Busy&&Cursor<History.Num())
    {
        Restore(History[Cursor++].After);
        Notice=TEXT("Snipped again.");
    }
}
void ASNIPPuzzle::Restart()
{
    if(!Busy)BuildPuzzle();
}
void ASNIPPuzzle::Tick(float Dt)
{
    Super::Tick(Dt);
    if(Busy)
    {
        Animation+=Dt;
        const float T=FMath::Clamp(Animation/FMath::Max(.05f,CompressionSeconds),0.f,1.f);
        const float E=1-FMath::Pow(1-T,3);
        for(int i=0; i<Cells.Num(); i++)Cells[i].Mesh->SetWorldLocation(FMath::Lerp(From[i],To[i],E));
        ExitMesh->SetWorldLocation(FMath::Lerp(ExitFrom,ExitPosition,E));
        if(auto* P=Player())P->SetActorLocation(FMath::Lerp(PlayerFrom,PlayerTo,E),false,nullptr,ETeleportType::TeleportPhysics);
        if(T>=1)
        {
            Busy=false;
            for(auto& C:Cells)C.Mesh->SetCollisionEnabled(C.Alive?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);
            if(auto* P=Player())
            {
                P->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
                P->GetCharacterMovement()->SetMovementMode(P->View?MOVE_None:MOVE_Falling);
            }
        }
        return;
    }
    auto* P=Player();
    if(!P)return;
    if(!P->View&&!Complete&&FVector::Dist(P->GetActorLocation(),ExitPosition)<130)
    {
        Complete=true;
        FinishTime=0;
        Notice=TEXT("PERFECT CUT!");
    }
    if(Complete)
    {
        FinishTime+=Dt;
        if(FinishTime>2.2f&&Stage<2)
        {
            ++Stage;
            BuildPuzzle();
        }
        return;
    }
    if(P->GetActorLocation().Z<GetActorLocation().Z-550)
    {
        P->SetActorLocation(Cursor?History[Cursor-1].After.Player:InitialState.Player,false,nullptr,ETeleportType::TeleportPhysics);
        P->GetCharacterMovement()->StopMovementImmediately();
        Notice=TEXT("Back on paper. Z undoes a cut; R restores this puzzle.");
    }
}
ASNIPCharacter::ASNIPCharacter()
{
    PrimaryActorTick.bCanEverTick=true;
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
    SetView(0);
}
bool ASNIPCharacter::CanMove() const
{
    return !View&&Puzzle&&!Puzzle->Busy&&!Puzzle->Complete;
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
    if(Puzzle&&Puzzle->Busy)return;
    View=FMath::Clamp(Mode,0,3);
    Cancel();
    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->SetMovementMode(View?MOVE_None:MOVE_Falling);
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
}
FVector ASNIPCharacter::MouseWorld() const
{
    FVector O,D;
    if(auto* PC=Cast<APlayerController>(Controller))PC->DeprojectMousePositionToWorld(O,D);
    return O;
}
void ASNIPCharacter::MouseDown()
{
    if(!View||!Puzzle||Puzzle->Busy||Puzzle->Complete)return;
    if(Locked)
    {
        Puzzle->ApplyCut(Axis,Low,High);
        return;
    }
    auto* PC=Cast<APlayerController>(Controller);
    PC->GetMousePosition(DragScreen.X,DragScreen.Y);
    DragWorld=MouseWorld();
    Selecting=true;
    AxisLocked=false;
    Low=High=0;
}
void ASNIPCharacter::MouseUp()
{
    if(Selecting)
    {
        Selecting=false;
        Locked=AxisLocked&&Low<High;
    }
}
void ASNIPCharacter::Cancel()
{
    Selecting=false;
    Locked=false;
    AxisLocked=false;
}
void ASNIPCharacter::Undo()
{
    if(Puzzle)Puzzle->Undo();
}
void ASNIPCharacter::Redo()
{
    if(Puzzle)Puzzle->Redo();
}
void ASNIPCharacter::Restart()
{
    if(Puzzle)Puzzle->Restart();
}
void ASNIPCharacter::Tick(float Dt)
{
    Super::Tick(Dt);
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
    ViewCamera->SetWorldLocation(FMath::VInterpTo(ViewCamera->GetComponentLocation(),Target,Dt,12));
    ViewCamera->SetWorldRotation(FMath::RInterpTo(ViewCamera->GetComponentRotation(),Rot,Dt,14));
    if(Selecting)
    {
        FVector2D Mouse;
        Cast<APlayerController>(Controller)->GetMousePosition(Mouse.X,Mouse.Y);
        FVector2D Delta=Mouse-DragScreen;
        if(!AxisLocked&&Delta.Size()>10)
        {
            bool Horizontal=FMath::Abs(Delta.X)>FMath::Abs(Delta.Y);
            Axis=Horizontal?(View==3?1:0):(View==2?1:2);
            AxisLocked=true;
        }
        if(AxisLocked)
        {
            // Reproject both endpoints through the same camera. Keeping a world
            // start point from a moving camera incorrectly expands the band.
            auto ProjectToPuzzle=[&](FVector2D Screen)
            {
                FVector Origin,Direction;
                Cast<APlayerController>(Controller)->DeprojectScreenPositionToWorld(Screen.X,Screen.Y,Origin,Direction);
                const int Hidden=View==1?1:View==2?2:0;
                const float Plane=Puzzle->GetActorLocation()[Hidden]+Puzzle->Dimensions[Hidden]*Puzzle->Spacing*.5f;
                return FMath::Abs(Direction[Hidden])>.001f?Origin+Direction*((Plane-Origin[Hidden])/Direction[Hidden]):Origin;
            };
            const FVector Start=ProjectToPuzzle(DragScreen),P=ProjectToPuzzle(Mouse);
            int A=FMath::RoundToInt((Start[Axis]-Puzzle->GetActorLocation()[Axis])/Puzzle->Spacing),B=FMath::RoundToInt((P[Axis]-Puzzle->GetActorLocation()[Axis])/Puzzle->Spacing);
            Low=FMath::Min(A,B);
            High=FMath::Max(A,B);
        }
    }
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
    DrawRect(FLinearColor(.97f,.94f,.84f,.93f),18,18,W-36,70*S);
    Text(TEXT("SNIP!"),36,28,1.8f,Ink);
    Text(G->Title(),200*S,37,1,Ink);
    Text(P->View==0?TEXT("1  PLAY     2  FRONT     3  TOP     4  SIDE"):P->View==1?TEXT("FRONT / X + Z"):P->View==2?TEXT("TOP / X + Y"):TEXT("SIDE / Y + Z"),36,105*S,.85f,Ink);
    DrawRect(FLinearColor(.97f,.94f,.84f,.94f),18,H-115*S,W-36,96*S);
    Text(P->View?TEXT("DRAG a band / RELEASE to lock / CLICK to snip / RMB cancel / 1 play"):TEXT("WASD walk / SPACE jump / 2 3 4 inspect & cut"),36,H-100*S,.75f,Ink);
    Text(FString::Printf(TEXT("Z undo (%d)   Y redo (%d)   R restart     %s"),G->Cursor,G->History.Num()-G->Cursor,*G->Notice),36,H-62*S,.66f,Ink);
    FString Hint=G->Stage==0?TEXT("Try TOP view. Remove the empty columns between the two islands."):G->Stage==1?TEXT("Try FRONT view. Snip paper layers below the steel platform."):TEXT("Width, depth AND height. Inspect TOP, then FRONT to connect the paper route.");
    Text(Hint,36,140*S,.72f,Ink);
    auto Project=[&](FVector V)
    {
        FVector2D O;
        GetOwningPlayerController()->ProjectWorldLocationToScreen(V,O);
        return O;
    };
    if(P->View)
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
            FLinearColor C=Why.IsEmpty()?Mint:Coral;
            FVector B=G->GetActorLocation(),E=B;
            B[P->Axis]+=P->Low*G->Spacing;
            E[P->Axis]+=P->High*G->Spacing;
            int Other=P->Axis==HA?VA:HA;
            E[Other]+=G->Dimensions[Other]*G->Spacing;
            auto Q=Project(B),R=Project(E);
            float X=FMath::Min(Q.X,R.X),Y=FMath::Min(Q.Y,R.Y),RW=FMath::Abs(Q.X-R.X),RH=FMath::Abs(Q.Y-R.Y);
            DrawRect(FLinearColor(C.R,C.G,C.B,.23f),X,Y,RW,RH);
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
            Text(FString::Printf(TEXT("%s [%d, %d) / %d cells   %s"),P->Axis==0?TEXT("X"):P->Axis==1?TEXT("Y"):TEXT("Z"),P->Low,P->High,P->High-P->Low,Why.IsEmpty()?TEXT("READY TO SNIP"):*Why),36,175*S,.8f,C);
        }
        float MX=0,MY=0;
        GetOwningPlayerController()->GetMousePosition(MX,MY);
        float Angle=P->Axis==VA?PI*.5f:0;
        auto V=[&](float x,float y)
        {
            return FVector2D(MX+FMath::Cos(Angle)*x-FMath::Sin(Angle)*y,MY+FMath::Sin(Angle)*x+FMath::Cos(Angle)*y);
        };
        float Spread=G->Busy?3:12;
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
    Text(TEXT("EXIT"),Exit.X-24,Exit.Y,.8f,Mint);
    if(G->Complete)
    {
        DrawRect(FLinearColor(.97f,.94f,.84f,.96f),W*.25f,H*.4f,W*.5f,110*S);
        Text(G->Stage==2?TEXT("PERFECT CUT!  ALL DONE."):TEXT("PERFECT CUT!"),W*.3f,H*.43f,1.5f,Ink);
    }
}
ASNIPGameMode::ASNIPGameMode()
{
    DefaultPawnClass=ASNIPCharacter::StaticClass();
    HUDClass=ASNIPHUD::StaticClass();
}
