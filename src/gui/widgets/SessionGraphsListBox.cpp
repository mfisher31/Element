/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "gui/widgets/SessionGraphsListBox.h"
#include "gui/ViewHelpers.h"

namespace element {

SessionGraphsListBox::SessionGraphsListBox (Session* s)
    : session (nullptr)
{
    setModel (this);
    updateContent();
}

SessionGraphsListBox::~SessionGraphsListBox()
{
    setModel (nullptr);
    session = nullptr;
}

int SessionGraphsListBox::getNumRows()
{
    return (session) ? session->getNumGraphs() : 0;
}

void SessionGraphsListBox::paintListBoxItem (int rowNumber, Graphics& g, int width, int height, bool rowIsSelected)
{
    if (! session)
        return;
    const Node node (session->getGraph (rowNumber));
    ViewHelpers::drawBasicTextRow ("  " + node.getName(), g, width, height, rowIsSelected);
}

} // namespace element
