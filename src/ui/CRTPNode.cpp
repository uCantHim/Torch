#include "trc/ui/CRTPNode.h"



namespace trc::ui
{

auto PublicTransformInterface::getPos() const -> vec2
{
    return localTransform.position;
}

auto PublicTransformInterface::getSize() const -> vec2
{
    return localTransform.size;
}

void PublicTransformInterface::setPos(vec2 newPos)
{
    localTransform.position = newPos;
}

void PublicTransformInterface::setPos(float x, float y)
{
    localTransform.position = { x, y };
}

void PublicTransformInterface::setPos(pix_or_norm x, pix_or_norm y)
{
    localTransform.position = { x.value, y.value };
    localTransform.positionFormat = { x.format, y.format };
}

void PublicTransformInterface::setPos(Vec2D<pix_or_norm> v)
{
    setPos(v.x, v.y);
}

void PublicTransformInterface::setSize(vec2 newSize)
{
    localTransform.size = newSize;
}

void PublicTransformInterface::setSize(float x, float y)
{
    localTransform.size = { x, y };
}

void PublicTransformInterface::setSize(pix_or_norm x, pix_or_norm y)
{
    localTransform.size = { x.value, y.value };
    localTransform.sizeFormat = { x.format, y.format };
}

void PublicTransformInterface::setSize(Vec2D<pix_or_norm> v)
{
    setSize(v.x, v.y);
}

auto PublicTransformInterface::getTransform() -> Transform
{
    return localTransform;
}

void PublicTransformInterface::setTransform(Transform newTransform)
{
    localTransform = newTransform;
}

auto PublicTransformInterface::getPositionFormat() const -> Vec2D<Format>
{
    return localTransform.positionFormat;
}

auto PublicTransformInterface::getSizeFormat() const -> Vec2D<Format>
{
    return localTransform.sizeFormat;
}

auto PublicTransformInterface::getPositionScaling() const -> Vec2D<Scale>
{
    return localTransform.positionScaling;
}

auto PublicTransformInterface::getSizeScaling() const -> Vec2D<Scale>
{
    return localTransform.sizeScaling;
}

void PublicTransformInterface::setPositionFormat(Vec2D<Format> newProps)
{
    localTransform.positionFormat = newProps;
}

void PublicTransformInterface::setSizeFormat(Vec2D<Format> newFormat)
{
    localTransform.sizeFormat = newFormat;
}

void PublicTransformInterface::setPositionScaling(Vec2D<Scale> newScaling)
{
    localTransform.positionScaling = newScaling;
}

void PublicTransformInterface::setSizeScaling(Vec2D<Scale> newScaling)
{
    localTransform.sizeScaling = newScaling;
}

void PublicTransformInterface::setPositioning(
    Vec2D<Format> newFormat,
    Vec2D<Scale> newScaling)
{
    localTransform.positionFormat = newFormat;
    localTransform.positionScaling = newScaling;
}

void PublicTransformInterface::setSizing(
    Vec2D<Format> newFormat,
    Vec2D<Scale> newScaling)
{
    localTransform.sizeFormat = newFormat;
    localTransform.sizeScaling = newScaling;
}

} // namespace trc::ui
