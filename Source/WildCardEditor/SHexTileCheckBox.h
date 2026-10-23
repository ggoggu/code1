#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/Input/SCheckBox.h"

class SHexTileCheckBox : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SHexTileCheckBox)
        : _IsChecked(ECheckBoxState::Unchecked)
        {
        }

        SLATE_ATTRIBUTE(ECheckBoxState, IsChecked)
        SLATE_EVENT(FOnCheckStateChanged, OnCheckStateChanged)

    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        CheckedState = InArgs._IsChecked;
        OnChanged = InArgs._OnCheckStateChanged;
    }

    virtual FVector2D ComputeDesiredSize(float) const override
    {
        return FVector2D(24.f, 24.f);
    }

    virtual int32 OnPaint(
        const FPaintArgs& Args,
        const FGeometry& Geometry,
        const FSlateRect& CullingRect,
        FSlateWindowElementList& DrawElements,
        int32 LayerId,
        const FWidgetStyle& WidgetStyle,
        bool bParentEnabled) const override
    {
        const FVector2D Center = Geometry.GetLocalSize() * 0.5f;
        const float Radius = GetRadius(Geometry);

        const ECheckBoxState State = CheckedState.Get();

        const bool bChecked = State != ECheckBoxState::Unchecked;
        const bool bPlayerStart = State == ECheckBoxState::Undetermined;

        FLinearColor Color;

        if (bPlayerStart)
        {
            Color = FLinearColor(1.f, 0.55f, 0.1f);
        }
        else if (bChecked)
        {
            Color = FLinearColor(0.15f, 0.9f, 0.45f);
        }
        else
        {
            Color = FLinearColor(0.45f, 0.45f, 0.45f);
        }

        Color *= WidgetStyle.GetColorAndOpacityTint();

        const ESlateDrawEffect Effects =
            ShouldBeEnabled(bParentEnabled)
            ? ESlateDrawEffect::None
            : ESlateDrawEffect::DisabledEffect;

        TArray<FVector2D> Outline;

        // 6개의 꼭짓점과 시작점을 연결해 육각형을 그립니다.
        for (int32 Index = 0; Index <= 6; ++Index)
        {
            const float Angle =
                FMath::DegreesToRadians(60.f * Index);

            Outline.Add(
                Center + FVector2D(
                    FMath::Cos(Angle) * Radius,
                    FMath::Sin(Angle) * Radius));
        }

        FSlateDrawElement::MakeLines(
            DrawElements,
            LayerId,
            Geometry.ToPaintGeometry(),
            Outline,
            Effects,
            Color,
            true,
            bChecked ? 2.f : 1.f);

        if (bChecked)
        {
            TArray<FVector2D> CheckMark;

            CheckMark.Add(
                Center + FVector2D(-0.4f, 0.f) * Radius);

            CheckMark.Add(
                Center + FVector2D(-0.1f, 0.3f) * Radius);

            CheckMark.Add(
                Center + FVector2D(0.45f, -0.35f) * Radius);

            FSlateDrawElement::MakeLines(
                DrawElements,
                LayerId + 1,
                Geometry.ToPaintGeometry(),
                CheckMark,
                Effects,
                Color,
                true,
                2.f);
        }

        return LayerId + 1;
    }

    virtual FReply OnMouseButtonDown(
        const FGeometry& Geometry,
        const FPointerEvent& MouseEvent) override
    {
        if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
        {
            return FReply::Unhandled();
        }

        const FVector2D Center = Geometry.GetLocalSize() * 0.5f;

        const FVector2D Position =
            Geometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition())
            - Center;

        const float Radius = GetRadius(Geometry);
        const float RootThree = FMath::Sqrt(3.f);

        const float X = FMath::Abs(Position.X);
        const float Y = FMath::Abs(Position.Y);

        // 사각 영역의 모서리가 아니라 육각형 내부만 클릭됩니다.
        const bool bInside =
            X <= Radius &&
            Y <= RootThree * Radius * 0.5f &&
            RootThree * X + Y <= RootThree * Radius;

        if (!bInside)
        {
            return FReply::Unhandled();
        }

        const ECheckBoxState NewState =
            CheckedState.Get() == ECheckBoxState::Checked
            ? ECheckBoxState::Unchecked
            : ECheckBoxState::Checked;

        OnChanged.ExecuteIfBound(NewState);

        Invalidate(EInvalidateWidgetReason::Paint);

        return FReply::Handled();
    }

private:
    static float GetRadius(const FGeometry& Geometry)
    {
        const FVector2D Size = Geometry.GetLocalSize();

        // 선이 위젯 경계 밖으로 나가지 않도록 여백을 둡니다.
        return FMath::Max(
            0.f,
            FMath::Min(
                Size.X * 0.5f,
                Size.Y / FMath::Sqrt(3.f)) - 2.f);
    }

    TAttribute<ECheckBoxState> CheckedState;
    FOnCheckStateChanged OnChanged;
};