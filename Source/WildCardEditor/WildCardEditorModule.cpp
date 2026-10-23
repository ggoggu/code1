#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailCustomization.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Text/STextBlock.h"

#include "InGame/Map/TileLayoutDataAsset.h"

#include "PropertyHandle.h"
#include "ScopedTransaction.h"
#include "UObject/UnrealType.h"

#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"

#include "IPropertyUtilities.h"
#include "SHexTileCheckBox.h"

// TileLayoutDataAsset의 Details 화면 구성
class FTileLayoutDetails : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance()
    {
        return MakeShared<FTileLayoutDetails>();
    }

    virtual void CustomizeDetails(
        IDetailLayoutBuilder& DetailBuilder) override
    {
        // 현재 Details에서 편집 중인 Data Asset을 가져옵니다.
        TArray<TWeakObjectPtr<UObject>> Objects;
        DetailBuilder.GetObjectsBeingCustomized(Objects);

        // 여러 에셋의 동시 편집은 지원하지 않습니다.
        if (Objects.Num() != 1)
        {
            return;
        }

        UTileLayoutDataAsset* Layout =
            Cast<UTileLayoutDataAsset>(Objects[0].Get());

        if (!Layout)
        {
            return;
        }

        const TWeakObjectPtr<UTileLayoutDataAsset> WeakLayout(Layout);

        // Width / Height가 바뀌면 격자를 다시 만듭니다.
        const TWeakPtr<IPropertyUtilities> WeakUtilities =
            DetailBuilder.GetPropertyUtilities();

        const FSimpleDelegate RefreshDelegate =
            FSimpleDelegate::CreateLambda([WeakUtilities]()
                {
                    if (const TSharedPtr<IPropertyUtilities> Utilities =
                        WeakUtilities.Pin())
                    {
                        Utilities->RequestForceRefresh();
                    }
                });

        DetailBuilder.GetProperty(
            GET_MEMBER_NAME_CHECKED(UTileLayoutDataAsset, Width))
            ->SetOnPropertyValueChanged(RefreshDelegate);

        DetailBuilder.GetProperty(
            GET_MEMBER_NAME_CHECKED(UTileLayoutDataAsset, Height))
            ->SetOnPropertyValueChanged(RefreshDelegate);

        // 원래 배열 입력창은 숨깁니다.
        DetailBuilder.HideProperty(
            GET_MEMBER_NAME_CHECKED(
                UTileLayoutDataAsset, EnabledCells));

        DetailBuilder.HideProperty(
            GET_MEMBER_NAME_CHECKED(
                UTileLayoutDataAsset, Player1Start));

        DetailBuilder.HideProperty(
            GET_MEMBER_NAME_CHECKED(
                UTileLayoutDataAsset, Player2Start));

        IDetailCategoryBuilder& Category =
            DetailBuilder.EditCategory(
                TEXT("TileLayoutEditor"),
                FText::FromString(TEXT("Tile Layout Editor")));

        TSharedRef<SVerticalBox> Grid =
            SNew(SVerticalBox);

        const int32 Rows = FMath::Clamp(Layout->Height, 1, 64);
        const int32 Columns = FMath::Clamp(Layout->Width, 1, 64);

        // 열 번호를 표시합니다.
        Grid->AddSlot()
            .AutoHeight()
            .Padding(0.f, 0.f, 0.f, 8.f)
            [
                SNew(SBox)
                    .WidthOverride(Columns * 28.f)
                    .HAlign(HAlign_Center)
                    [
                        SNew(STextBlock)
                            .Text(FText::FromString(
                                FString::Printf(TEXT("Width[%d]"), Columns)))
                    ]
            ];

        // 행별로 체크박스를 만듭니다.
        for (int32 Row = 0; Row < Rows; ++Row)
        {
            TSharedRef<SHorizontalBox> RowWidget =
                SNew(SHorizontalBox);

            for (int32 Column = 0; Column < Columns; ++Column)
            {
                const FIntPoint Cell(Row, Column);

                RowWidget->AddSlot()
                    .AutoWidth()
                    [
                        SNew(SBox)
                        .WidthOverride(28.f)
                        .HeightOverride(28.f)
                        .RenderTransform(
                            FSlateRenderTransform(
                                FVector2D(0.f, Column % 2 != 0 ? 14.f : 0.f)))
                        .HAlign(HAlign_Center)
                        .VAlign(VAlign_Center)
                        [
                            SNew(SHexTileCheckBox)

                                .ToolTipText_Lambda([WeakLayout, Cell]()
                                    {
                                        const UTileLayoutDataAsset* Asset = WeakLayout.Get();

                                        if (Asset)
                                        {
                                            if (Asset->Player1Start == Cell)
                                            {
                                                return FText::FromString(TEXT("Player 1 Start"));
                                            }

                                            if (Asset->Player2Start == Cell)
                                            {
                                                return FText::FromString(TEXT("Player 2 Start"));
                                            }
                                        }

                                        return FText::FromString(
                                            FString::Printf(
                                                TEXT("Row %d, Column %d"),
                                                Cell.X, Cell.Y));
                                    })

                                .IsChecked_Lambda([WeakLayout, Cell]()
                                {
                                    const UTileLayoutDataAsset* Asset = WeakLayout.Get();

                                    if (!Asset)
                                    {
                                        return ECheckBoxState::Unchecked;
                                    }

                                    if (Asset->Player1Start == Cell ||
                                        Asset->Player2Start == Cell)
                                    {
                                        return ECheckBoxState::Undetermined;
                                    }

                                    return Asset->EnabledCells.Contains(Cell)
                                        ? ECheckBoxState::Checked
                                        : ECheckBoxState::Unchecked;
                                })

                                .OnCheckStateChanged_Lambda(
                                    [WeakLayout, Cell](ECheckBoxState)
                                    {
                                        UTileLayoutDataAsset* Asset = WeakLayout.Get();

                                        if (!Asset)
                                        {
                                            return;
                                        }

                                        const FIntPoint Unassigned(-1, -1);

                                        const FScopedTransaction Transaction(
                                            FText::FromString(TEXT("Edit Tile Layout")));

                                        Asset->Modify();

                                        // 한 번의 클릭으로 여러 속성이 함께 변경될 수 있습니다.
                                        Asset->PreEditChange(nullptr);

                                        const bool bPlayer1Start = Asset->Player1Start == Cell;
                                        const bool bPlayer2Start = Asset->Player2Start == Cell;
                                        const bool bEnabled = Asset->EnabledCells.Contains(Cell);

                                        if (bPlayer1Start || bPlayer2Start)
                                        {
                                            // 주황색 → 회색
                                            if (bPlayer1Start)
                                            {
                                                Asset->Player1Start = Unassigned;
                                            }

                                            if (bPlayer2Start)
                                            {
                                                Asset->Player2Start = Unassigned;
                                            }

                                            Asset->EnabledCells.Remove(Cell);
                                        }
                                        else if (!bEnabled)
                                        {
                                            // 회색 → 초록색
                                            Asset->EnabledCells.AddUnique(Cell);
                                        }
                                        else if (Asset->Player1Start == Unassigned)
                                        {
                                            // 초록색 → P1 시작 위치
                                            Asset->Player1Start = Cell;
                                        }
                                        else if (Asset->Player2Start == Unassigned)
                                        {
                                            // 초록색 → P2 시작 위치
                                            Asset->Player2Start = Cell;
                                        }
                                        else
                                        {
                                            // 시작 위치가 이미 2개라면 초록색 → 회색
                                            Asset->EnabledCells.Remove(Cell);
                                        }

                                        Asset->PostEditChange();
                                        Asset->MarkPackageDirty();
                                    })
                        ]
                    ];
            }

            Grid->AddSlot()
                .AutoHeight()
                [
                    RowWidget
                ];
        }
        TSharedRef<SHorizontalBox> LabeledGrid =
            SNew(SHorizontalBox)

            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0.f, 0.f, 12.f, 0.f)
            [
                SNew(STextBlock)
                    .Text(FText::FromString(
                        FString::Printf(TEXT("Height[%d]"), Rows)))
            ]

            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                Grid
            ];

        Category.AddCustomRow(
            FText::FromString(TEXT("Layout Grid")))
        .WholeRowContent()
        [
            SNew(SBox)
            .MaxDesiredHeight(450.f)
            [
                SNew(SScrollBox)
                + SScrollBox::Slot()
                [
                    SNew(SScrollBox)
                    .Orientation(Orient_Horizontal)
                    
                    + SScrollBox::Slot()
                    .Padding(0.f, 0.f, 0.f, 16.f)
                    [
                        LabeledGrid
                    ]
                ]
            ]
        ];
    }
};
// 에디터가 시작될 때 위 Details 구성을 등록합니다.
class FWildCardEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        FPropertyEditorModule& PropertyEditor =
            FModuleManager::LoadModuleChecked<FPropertyEditorModule>(
                TEXT("PropertyEditor"));

        PropertyEditor.RegisterCustomClassLayout(
            UTileLayoutDataAsset::StaticClass()->GetFName(),
            FOnGetDetailCustomizationInstance::CreateStatic(
                &FTileLayoutDetails::MakeInstance));

        PropertyEditor.NotifyCustomizationModuleChanged();
    }

    virtual void ShutdownModule() override
    {
        if (FModuleManager::Get().IsModuleLoaded(
            TEXT("PropertyEditor")))
        {
            FPropertyEditorModule& PropertyEditor =
                FModuleManager::GetModuleChecked<FPropertyEditorModule>(
                    TEXT("PropertyEditor"));

            PropertyEditor.UnregisterCustomClassLayout(
                TEXT("TileLayoutDataAsset"));

            PropertyEditor.NotifyCustomizationModuleChanged();
        }
    }
};

IMPLEMENT_MODULE(FWildCardEditorModule, WildCardEditor)