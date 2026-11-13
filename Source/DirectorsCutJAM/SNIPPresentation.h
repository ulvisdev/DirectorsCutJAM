#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SNIPPresentation.generated.h"
class ASNIPCharacter;
class AHUD;
class UCanvas;
class USoundBase;

// Session-scoped tutorial, menus and a single midpoint-commit transition clock.
// World edits remain owned by SNIPPuzzle; this component only orchestrates them.
UCLASS(ClassGroup=SNIP,meta=(BlueprintSpawnableComponent))
class DIRECTORSCUTJAM_API USNIPPresentation : public UActorComponent
{
    GENERATED_BODY()
public:
    USNIPPresentation();
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="SNIP|Transitions") float ViewSeconds=.20f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="SNIP|Transitions") float DeathSeconds=.42f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="SNIP|Transitions") float RestartSeconds=.32f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="SNIP|Transitions") float SceneSeconds=.55f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="SNIP|Hints") int32 UndoHintThreshold=3;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="SNIP|Hints") int32 RestartHintThreshold=2;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="SNIP|Audio") TObjectPtr<USoundBase> CameraSound;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="SNIP|Audio") TObjectPtr<USoundBase> SpliceSound;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="SNIP|Audio") TObjectPtr<USoundBase> RestartSound;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="SNIP|Audio") TObjectPtr<USoundBase> SceneSound;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="SNIP|Audio") TObjectPtr<USoundBase> TutorialSound;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="SNIP|Audio") TObjectPtr<USoundBase> MenuSound;
    UPROPERTY(BlueprintReadOnly) bool MenuOpen=true;
    UPROPERTY(BlueprintReadOnly) bool TitleMenu=true;
    UPROPERTY(BlueprintReadOnly) int32 MenuPage=0;
    UPROPERTY(BlueprintReadOnly) int32 TutorialStep=0;
    UPROPERTY(BlueprintReadOnly) bool TransitionActive=false;
    UPROPERTY(BlueprintReadOnly) bool TutorialBlocking=false;
    UPROPERTY(BlueprintReadOnly) FString Prompt;
    UPROPERTY(BlueprintReadOnly) int32 Deaths=0;
    UFUNCTION(BlueprintCallable) void StartView(int32 Mode);
    UFUNCTION(BlueprintCallable) void Retry(bool Death=false);
    UFUNCTION(BlueprintCallable) void NextScene();
    UFUNCTION(BlueprintCallable) void ToggleMenu();
    UFUNCTION(BlueprintCallable) void Accept();
    UFUNCTION(BlueprintCallable) void SkipTutorial();
    void Up(); void Down(); void Click();
    void Initialize();
    void Update(float Dt);
    void Draw(AHUD* HUD,UCanvas* Canvas);
    void CutResult(bool Valid,const FString& Reason);
    void HistoryUsed(bool Undo);
    void Hint(const FString& Text,float Seconds=3.f);
    bool BlocksMovement() const {return MenuOpen||TransitionActive||TutorialBlocking;}
    bool BlocksEditing() const {return MenuOpen||TransitionActive;}
    bool Applying=false;
    float GetVolume() const {return Volume;}
    bool SnapCamera=false;
private:
    ASNIPCharacter* Player() const;
    void Start(int32 Kind,float Duration,int32 View=0);
    void SetTutorial(int32 Step);
    void Sound(USoundBase* Asset);
    void Cursor();
    TArray<FString> Rows() const;
    int32 TransitionKind=0,TargetView=0,Selected=0,Cuts=0,Failures=0;
    float Elapsed=0,Duration=1,HintTime=0,PromptAlpha=0;
    float Volume=1;
    bool Midpoint=false,UndoHintSeen=false,RedoHintSeen=false,MetalHintSeen=false,ViewsHintSeen=false,ShakeEnabled=true;
    FString HintText,DisplayedPrompt;
    FVector TutorialOrigin=FVector::ZeroVector;
};
