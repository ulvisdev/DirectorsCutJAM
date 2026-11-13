#include "SNIPPresentation.h"
#include "SNIP.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/InputSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Sound/SoundBase.h"

USNIPPresentation::USNIPPresentation()
{
    // Existing temporary sounds; all references can be replaced on the component.
    CameraSound=LoadObject<USoundBase>(nullptr,TEXT("/Game/SNIP/Audio/SNIP_Snip.SNIP_Snip"));
    SpliceSound=CameraSound;
    RestartSound=LoadObject<USoundBase>(nullptr,TEXT("/Game/SNIP/Audio/SNIP_Undo.SNIP_Undo"));
    SceneSound=LoadObject<USoundBase>(nullptr,TEXT("/Game/SNIP/Audio/SNIP_Slide.SNIP_Slide"));
    TutorialSound=LoadObject<USoundBase>(nullptr,TEXT("/Game/SNIP/Audio/SNIP_Redo.SNIP_Redo"));
    MenuSound=TutorialSound;
}
ASNIPCharacter* USNIPPresentation::Player() const {return Cast<ASNIPCharacter>(GetOwner());}
void USNIPPresentation::Sound(USoundBase* Asset)
{
    if(Asset)UGameplayStatics::PlaySound2D(this,Asset,.25f*Volume);
}
void USNIPPresentation::Cursor()
{
    auto* P=Player();if(!P)return;
    if(auto* PC=Cast<APlayerController>(P->GetController()))
    {
        PC->bShowMouseCursor=MenuOpen||P->View;
        if(PC->bShowMouseCursor)PC->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
        else PC->SetInputMode(FInputModeGameOnly());
    }
}
void USNIPPresentation::Initialize()
{
    if(auto* P=Player()){TutorialOrigin=P->GetActorLocation();P->FreezeMovement();}
    Cursor();
}
void USNIPPresentation::Start(int32 Kind,float Seconds,int32 Mode)
{
    if(TransitionActive)return;
    TransitionKind=Kind;Duration=FMath::Max(.08f,Seconds);TargetView=Mode;
    Elapsed=0;Midpoint=false;TransitionActive=true;
    if(auto* P=Player()){P->Cancel();P->FreezeMovement();}
    Sound(Kind==0?CameraSound:Kind==1?RestartSound:SceneSound);
}
void USNIPPresentation::StartView(int32 Mode)
{
    auto* P=Player();if(!P||MenuOpen||TransitionActive||!P->Puzzle||P->Puzzle->Busy)return;
    Mode=FMath::Clamp(Mode,0,3);
    if(Mode==P->View)return;
    Start(0,ViewSeconds,Mode);
}
void USNIPPresentation::Retry(bool Death)
{
    auto* P=Player();if(!P||!P->Puzzle||P->Puzzle->Busy||TransitionActive||MenuOpen)return;
    if(Death)++Deaths;
    Start(1,Death?DeathSeconds:RestartSeconds);
}
void USNIPPresentation::NextScene()
{
    if(!TransitionActive&&!MenuOpen)Start(2,SceneSeconds);
}
void USNIPPresentation::Hint(const FString& Text,float Seconds)
{
    HintText=Text;HintTime=Seconds;
}
void USNIPPresentation::SetTutorial(int32 Step)
{
    if(Step<=TutorialStep)return;
    TutorialStep=Step;TutorialBlocking=false;Sound(TutorialSound);
}
void USNIPPresentation::SkipTutorial()
{
    SetTutorial(5);TutorialBlocking=false;HintTime=0;
    if(auto* P=Player())if(!P->View&&!BlocksMovement())P->ResumeMovement();
}
void USNIPPresentation::CutResult(bool Valid,const FString& Reason)
{
    if(Valid)
    {
        ++Cuts;if(TutorialStep<4)SetTutorial(4);
        if(Cuts>=UndoHintThreshold&&!UndoHintSeen){UndoHintSeen=true;Hint(TEXT("Z — UNDO"));}
    }
    else
    {
        ++Failures;
        if(Reason.Contains(TEXT("Metal"))){MetalHintSeen=true;Hint(TEXT("METAL CAN'T BE CUT"));}
        else Hint(Reason);
        if(Failures>=UndoHintThreshold&&!UndoHintSeen){UndoHintSeen=true;Hint(TEXT("Z — UNDO"));}
    }
}
void USNIPPresentation::HistoryUsed(bool Undo)
{
    if(Undo&&!RedoHintSeen){RedoHintSeen=true;Hint(TEXT("Y — REDO"));}
}
void USNIPPresentation::Update(float Dt)
{
    auto* P=Player();if(!P||!P->Puzzle)return;
    auto* G=P->Puzzle.Get();
    if(TransitionActive)
    {
        Elapsed+=Dt;
        if(!Midpoint&&Elapsed>=Duration*.5f)
        {
            Midpoint=true;Applying=true;Sound(SpliceSound);
            if(TransitionKind==0)P->ApplyView(TargetView);
            else if(TransitionKind==1 || G->Stage<4)
            {
                if(TransitionKind==2)++G->Stage;
                G->BuildPuzzle();P->FreezeMovement();
                TutorialOrigin=P->GetActorLocation();
                TutorialBlocking=false;
                if(TransitionKind==1 && Deaths>=RestartHintThreshold)Hint(TEXT("R — RESTART"));
                if(G->Stage==1&&!MetalHintSeen){MetalHintSeen=true;Hint(TEXT("STEEL SURVIVES. CUT THE PAPER BENEATH."),4);}
                if(G->Stage==2&&!ViewsHintSeen){ViewsHintSeen=true;Hint(TEXT("3 — TOP VIEW     4 — SIDE VIEW"),4);}
            }
            else
            {
                TitleMenu=true;MenuOpen=true;MenuPage=0;Selected=0;
                Hint(TEXT("FIN"),4);
            }
            SnapCamera=true;Applying=false;
        }
        if(Elapsed>=Duration)
        {
            TransitionActive=false;
            if(!P->View&&!MenuOpen&&!TutorialBlocking)P->ResumeMovement();
            Cursor();
        }
    }
    HintTime=FMath::Max(0.f,HintTime-Dt);
    if(!MenuOpen&&!TransitionActive&&G->Stage==0)
    {
        if(TutorialStep==0&&FVector::Dist2D(P->GetActorLocation(),TutorialOrigin)>60)SetTutorial(1);
        if(TutorialStep==1&&P->GetCharacterMovement()->IsFalling()&&P->GetVelocity().Z>50)SetTutorial(2);
        // Skipping the little prop never traps the player: reaching the gap also advances.
        if(TutorialStep<3&&P->GetActorLocation().X>G->GetActorLocation().X+650)
        {
            SetTutorial(2);TutorialBlocking=true;P->FreezeMovement();
        }
        if(P->View&&TutorialStep<3)SetTutorial(3);
        if(TutorialStep==4&&!P->View)SetTutorial(5);
    }
    FString Want;
    if(!MenuOpen)
    {
        if(G->Stage==0&&TutorialStep<5)
        {
            if(TutorialStep==0)Want=TEXT("WASD — MOVE");
            if(TutorialStep==1&&P->GetActorLocation().X>G->GetActorLocation().X+340)Want=TEXT("SPACE — JUMP");
            if(TutorialStep==2&&TutorialBlocking)Want=TEXT("CHANGE YOUR PERSPECTIVE\n2 — FRONT VIEW");
            if(TutorialStep==3)Want=P->View?TEXT("HOLD • DRAG • RELEASE"):TEXT("2 — FRONT VIEW");
            if(TutorialStep==4)Want=TEXT("1 — RETURN TO THE FILM");
        }
        if(Want.IsEmpty()&&HintTime>0)Want=HintText;
    }
    // Fade the old prompt fully before replacing it; no hard text pop.
    if(Want!=DisplayedPrompt)
    {
        PromptAlpha=FMath::Max(0.f,PromptAlpha-Dt*6);
        if(PromptAlpha<=0)DisplayedPrompt=Want;
    }
    else PromptAlpha=FMath::Min(1.f,PromptAlpha+Dt*6);
    Prompt=DisplayedPrompt;
}
TArray<FString> USNIPPresentation::Rows() const
{
    if(MenuPage==1)return {TEXT("BACK")};
    if(MenuPage==2)return {FString::Printf(TEXT("SOUND   %s"),Volume>0?TEXT("ON"):TEXT("OFF")),FString::Printf(TEXT("CAMERA SHAKE   %s"),ShakeEnabled?TEXT("ON"):TEXT("OFF")),TEXT("SKIP TUTORIAL"),TEXT("BACK")};
    return {TitleMenu?TEXT("PLAY"):TEXT("RESUME"),TEXT("CONTROLS"),TEXT("SETTINGS"),TEXT("QUIT")};
}
void USNIPPresentation::ToggleMenu()
{
    auto* P=Player();if(!P||TransitionActive||P->Puzzle&&P->Puzzle->Busy)return;
    Sound(MenuSound);
    if(MenuOpen&&MenuPage){MenuPage=0;Selected=0;return;}
    if(MenuOpen&&TitleMenu)return;
    MenuOpen=!MenuOpen;Selected=0;MenuPage=0;
    P->Cancel();
    if(MenuOpen)P->FreezeMovement();
    else if(!P->View&&!TutorialBlocking)P->ResumeMovement();
    Cursor();
}
void USNIPPresentation::Up(){if(MenuOpen){Selected=(Selected+Rows().Num()-1)%Rows().Num();Sound(MenuSound);}}
void USNIPPresentation::Down(){if(MenuOpen){Selected=(Selected+1)%Rows().Num();Sound(MenuSound);}}
void USNIPPresentation::Accept()
{
    auto* P=Player();if(!P||!MenuOpen)return;
    Sound(MenuSound);
    if(MenuPage==1){MenuPage=0;Selected=0;return;}
    if(MenuPage==2)
    {
        if(Selected==0)Volume=Volume>0?0:1;
        if(Selected==1){ShakeEnabled=!ShakeEnabled;if(P->Puzzle)P->Puzzle->CameraShakeIntensity=ShakeEnabled?2:0;}
        if(Selected==2)SkipTutorial();
        if(Selected==3){MenuPage=0;Selected=0;}
        return;
    }
    if(Selected==0)
    {
        if(TitleMenu&&P->Puzzle&&P->Puzzle->Complete){P->Puzzle->Stage=0;Applying=true;P->Puzzle->BuildPuzzle();Applying=false;}
        TitleMenu=false;MenuOpen=false;Cursor();
        if(!P->View&&!TutorialBlocking)P->ResumeMovement();
    }
    else if(Selected==1){MenuPage=1;Selected=0;}
    else if(Selected==2){MenuPage=2;Selected=0;}
    else UKismetSystemLibrary::QuitGame(this,Cast<APlayerController>(P->GetController()),EQuitPreference::Quit,false);
}
void USNIPPresentation::Click()
{
    if(!MenuOpen)return;
    auto* PC=Cast<APlayerController>(Player()->GetController());if(!PC)return;
    int W,H;PC->GetViewportSize(W,H);float X,Y;if(!PC->GetMousePosition(X,Y))return;
    const float Start=MenuPage==1?H*.86f:H*.48f,Step=FMath::Min(56.f,H*.065f);
    const int Row=FMath::FloorToInt((Y-Start)/Step);
    if(X>W*.3f&&X<W*.7f&&Row>=0&&Row<Rows().Num()){Selected=Row;Accept();}
}
void USNIPPresentation::Draw(AHUD* HUD,UCanvas* Canvas)
{
    const float W=Canvas->SizeX,H=Canvas->SizeY,Scale=FMath::Clamp(H/900.f,.65f,1.4f);
    const FLinearColor Cream(.949f,.894f,.784f),Brown(.349f,.271f,.212f),Orange(.851f,.471f,.259f);
    auto CenterText=[&](const FString& Text,float Y,float Size,FLinearColor Color)
    {
        float TW,TH;HUD->GetTextSize(Text,TW,TH,GEngine->GetLargeFont(),Size*Scale);
        HUD->DrawText(Text,Color,(W-TW)*.5f,Y,GEngine->GetLargeFont(),Size*Scale);
    };
    if(MenuOpen)
    {
        HUD->DrawRect(Brown,0,0,W,H);
        HUD->DrawRect(Cream,W*.12f,H*.09f,W*.76f,H*.82f);
        CenterText(MenuPage==1?TEXT("CONTROLS"):MenuPage==2?TEXT("SETTINGS"):TEXT("SNIP!"),H*.19f,3.5f,Brown);
        if(MenuPage==0)CenterText(TEXT("A HANDMADE FILM"),H*.32f,.9f,Brown);
        if(MenuPage==1)
        {
            // Generate bindings from the actual project InputSettings, not a second key list.
            TMap<FName,FString> Labels={{TEXT("SNIPJump"),TEXT("JUMP")},{TEXT("SNIPGame"),TEXT("RETURN TO FILM")},{TEXT("SNIPFront"),TEXT("FRONT VIEW")},{TEXT("SNIPTop"),TEXT("TOP VIEW")},{TEXT("SNIPSide"),TEXT("SIDE VIEW")},{TEXT("SNIPSelect"),TEXT("HOLD / DRAG / RELEASE TO CUT")},{TEXT("SNIPCancel"),TEXT("CANCEL CUT")},{TEXT("SNIPUndo"),TEXT("UNDO")},{TEXT("SNIPRedo"),TEXT("REDO")},{TEXT("SNIPRestart"),TEXT("RESTART")},{TEXT("SNIPMenu"),TEXT("PAUSE / BACK")},{TEXT("SNIPSkip"),TEXT("DISMISS TUTORIAL")}};
            TArray<FString> Lines;FString Move;
            for(const auto& M:GetDefault<UInputSettings>()->GetAxisMappings())if(M.AxisName.ToString().StartsWith(TEXT("SNIP")))Move+=M.Key.GetDisplayName().ToString()+TEXT("  ");
            Lines.Add(Move+TEXT("— MOVE"));
            for(const auto& M:GetDefault<UInputSettings>()->GetActionMappings())if(Labels.Contains(M.ActionName))Lines.Add(M.Key.GetDisplayName().ToString()+TEXT(" — ")+Labels[M.ActionName]);
            float Y=H*.30f;
            for(const auto& Line:Lines){CenterText(Line,Y,.86f,Brown);Y+=H*.037f;}
        }
        const auto Items=Rows();const float Start=MenuPage==1?H*.86f:H*.48f,Step=FMath::Min(56.f,H*.065f);
        for(int i=0;i<Items.Num();++i)CenterText((Selected==i?TEXT("•  "):TEXT(""))+Items[i],Start+i*Step,1.3f,Selected==i?Orange:Brown);
    }
    else if(!DisplayedPrompt.IsEmpty()&&PromptAlpha>0)
    {
        const bool Card=TutorialBlocking;
        const float Y=Card?H*.40f:H*.84f;
        const float BW=FMath::Min(W*.8f,620*Scale),BH=Card?140*Scale:44*Scale;
        HUD->DrawRect(FLinearColor(Cream.R,Cream.G,Cream.B,PromptAlpha*.94f),(W-BW)*.5f,Y-12*Scale,BW,BH);
        TArray<FString> Lines;DisplayedPrompt.ParseIntoArrayLines(Lines);
        for(int i=0;i<Lines.Num();++i)CenterText(Lines[i],Y+i*38*Scale,Card?1.3f:1.05f,FLinearColor(Brown.R,Brown.G,Brown.B,PromptAlpha));
        if(Card)CenterText(TEXT("TAB — DISMISS"),Y+90*Scale,.65f,FLinearColor(Brown.R,Brown.G,Brown.B,PromptAlpha));
    }
    if(!MenuOpen&&Player()&&Player()->View)
    {
        for(float X:{14.f,W-22.f})for(float Y=18;Y<H-18;Y+=42)HUD->DrawRect(FLinearColor(Cream.R,Cream.G,Cream.B,.32f),X,Y,8,18);
    }
    if(TransitionActive)
    {
        const float T=FMath::Clamp(Elapsed/Duration,0.f,1.f);
        // Fully opaque around the midpoint, so rebuilding happens behind the splice.
        const float Cover=T<.4f?T/.4f:T>.6f?(1-T)/.4f:1.f;
        if(TransitionKind==2)
        {
            // A solid paper sheet sweeps in, hides the area swap, then exits.
            const float Left=T<=.6f?0:FMath::Clamp((T-.6f)/.4f,0.f,1.f)*W;
            const float Right=T<.4f?T/.4f*W:W;
            HUD->DrawRect(Cream,Left,0,Right-Left,H);
            for(float Y=0;Y<H;Y+=14)
                HUD->DrawRect(Cream,Right-1,Y,4+2*FMath::Sin(Y*.37f),14);
            HUD->DrawRect(FLinearColor(Brown.R,Brown.G,Brown.B,.18f),Left,0,3,H);
        }
        else
        {
            HUD->DrawRect(FLinearColor(Cream.R,Cream.G,Cream.B,FMath::Clamp(Cover,0.f,1.f)),0,0,W,H);
            HUD->DrawRect(FLinearColor(Brown.R,Brown.G,Brown.B,Cover*.5f),0,H*T, W,3);
        }
    }
}
