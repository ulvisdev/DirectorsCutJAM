#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "SNIP.generated.h"
class UStaticMeshComponent;
class UCameraComponent;
USTRUCT(BlueprintType)
struct FSNIPCell
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FIntVector Grid=FIntVector::ZeroValue;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool Metal=false;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool Alive=true;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Mesh;
};
struct FSNIPState
{
    TArray<FIntVector> Positions;
    TArray<bool> Alive;
    FIntVector Size;
    FVector Player;
    FVector Exit;
};
// Each command owns both snapshots: walking between cuts must not overwrite
// the original result that Redo is expected to reproduce.
struct FSNIPCommand
{
    FSNIPState Before;
    FSNIPState After;
};
UCLASS()
class DIRECTORSCUTJAM_API ASNIPPuzzle : public AActor
{
    GENERATED_BODY()
    public:
    ASNIPPuzzle();
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="SNIP") float Spacing=200;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="SNIP|Art") TObjectPtr<class UMaterialInterface> PaperMaterial;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="SNIP|Art") TObjectPtr<class UMaterialInterface> MetalMaterial;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="SNIP") float CompressionSeconds=.20f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="SNIP|Feel") float SnipSeconds=.08f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="SNIP|Feel") float AnticipationSeconds=.08f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="SNIP|Feel") float ImpactSeconds=.07f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="SNIP|Feel") float AnticipationTiles=.04f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="SNIP|Feel") float CompressionExponent=3.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="SNIP|Feel") float VibrationAmplitude=2.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="SNIP|Feel") float CameraShakeIntensity=2.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="SNIP|Feel") int32 ImpactParticleCount=12;
    float ImpactPulse=0;
    FVector Seam=FVector::ZeroVector;
    static float CompressCoordinate(float Value,float A,float B);
    FVector SafePlayerPosition(FVector Desired) const;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="SNIP") int32 Stage=0;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="SNIP") FIntVector Dimensions;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="SNIP") FVector ExitPosition;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="SNIP") TArray<FSNIPCell> Cells;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> ExitMesh;
    UFUNCTION(BlueprintCallable,CallInEditor,Category="SNIP") void BuildPuzzle();
    UFUNCTION(BlueprintCallable,Category="SNIP") bool ApplyCut(int32 Axis,int32 Start,int32 End);
    UFUNCTION(BlueprintCallable,Category="SNIP") void Undo();
    UFUNCTION(BlueprintCallable,Category="SNIP") void Redo();
    UFUNCTION(BlueprintCallable,Category="SNIP") void Restart();
    UFUNCTION(BlueprintCallable,Category="SNIP") FString ValidateCut(int32 Axis,int32 Start,int32 End) const;
    UPROPERTY(BlueprintReadOnly) bool Busy=false;
    UPROPERTY(BlueprintReadOnly) bool Complete=false;
    UPROPERTY(BlueprintReadOnly) FString Notice;
    int32 Cursor=0;
    TArray<FSNIPCommand> History;
    FSNIPState InitialState;
    FVector SpawnPoint;
    class ASNIPCharacter* Player() const;
    FVector CellWorld(FIntVector P) const;
    FString Title() const;
    private:
    FSNIPState Capture() const;
    void Restore(const FSNIPState& State,int32 Style=0);
    void AddCell(FIntVector P,bool Metal=false);
    TArray<FVector> From,To;
    FVector PlayerFrom,PlayerTo,ExitFrom;
    float Animation=0,FinishTime=0;
    int32 AnimationStyle=0;
    bool ImpactPlayed=false;
};
UCLASS()
class DIRECTORSCUTJAM_API ASNIPCharacter : public ACharacter
{
    GENERATED_BODY()
    public:
    ASNIPCharacter();
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> ViewCamera;
    UPROPERTY(BlueprintReadOnly) int32 View=0;
    UPROPERTY(BlueprintReadOnly) int32 Axis=0;
    UPROPERTY(BlueprintReadOnly) int32 Low=0;
    UPROPERTY(BlueprintReadOnly) int32 High=0;
    UPROPERTY(BlueprintReadOnly) bool Selecting=false;
    UPROPERTY(BlueprintReadOnly) bool Locked=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="SNIP|Input") float DragThreshold=7.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="SNIP|Input") float DirectionHysteresis=1.25f;
    FVector FrozenVelocity=FVector::ZeroVector;
    bool MovementFrozen=false;
    EMovementMode FrozenMode=MOVE_Walking;
    uint8 FrozenCustomMode=0;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<class USNIPPresentation> Presentation;
    float ScissorFeedback=0;
    bool Rejected=false;
    void FreezeMovement();
    void ResumeMovement();
    void UpdateSelection();
    UPROPERTY() TObjectPtr<ASNIPPuzzle> Puzzle;
    UFUNCTION(BlueprintCallable) void SetView(int32 Mode);
    void ApplyView(int32 Mode);
    void MouseDown();
    void MouseUp();
    void Cancel();
    void GameView()
    {
        SetView(0);
    }
    void Front()
    {
        SetView(1);
    }
    void Top()
    {
        SetView(2);
    }
    void Side()
    {
        SetView(3);
    }
    void Undo();
    void Redo();
    void Restart();
    void JumpPressed();
    void MoveForward(float V);
    void MoveRight(float V);
    FVector MouseWorld() const;
    bool CanMove() const;
    private:
    FVector DragWorld;
    FVector2D DragScreen;
    bool AxisLocked=false;
};
UCLASS()
class DIRECTORSCUTJAM_API ASNIPHUD : public AHUD
{
    GENERATED_BODY()
    public: virtual void DrawHUD() override;
};
UCLASS()
class DIRECTORSCUTJAM_API ASNIPGameMode : public AGameModeBase
{
    GENERATED_BODY()
    public: ASNIPGameMode();
};
