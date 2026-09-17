modded class WidgetEventHandler
{
    override bool OnMouseWheel(Widget w, int x, int y, int wheel)
    {
        if (TransferZOperationDrag.IsModifierItemDrag())
        {
            string widgetName = "<null>";
            if (w)
                widgetName = w.GetName();
            int mouseX;
            int mouseY;
            GetMousePos(mouseX, mouseY);
            Print("[TransferZ][DragDiag] WHEEL widget=" + widgetName + " event=" + x.ToString() + "," + y.ToString() + " actual=" + mouseX.ToString() + "," + mouseY.ToString() + " wheel=" + wheel.ToString());
        }

        return super.OnMouseWheel(w, x, y, wheel);
    }

    override bool OnDropReceived(Widget w, int x, int y, Widget reciever)
    {
        if (TransferZOperationDrag.IsModifierItemDrag())
        {
            string sourceName = "<null>";
            string receiverName = "<null>";
            if (w)
                sourceName = w.GetName();
            if (reciever)
                receiverName = reciever.GetName();
            int mouseX;
            int mouseY;
            GetMousePos(mouseX, mouseY);
            Print("[TransferZ][DragDiag] DROP_RECEIVED source=" + sourceName + " receiver=" + receiverName + " event=" + x.ToString() + "," + y.ToString() + " actual=" + mouseX.ToString() + "," + mouseY.ToString());
        }

        return super.OnDropReceived(w, x, y, reciever);
    }
}
