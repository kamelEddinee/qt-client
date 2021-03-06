/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#ifndef DSPPOITEMRECEIVINGSBYVENDOR_H
#define DSPPOITEMRECEIVINGSBYVENDOR_H

#include "display.h"

#include "ui_dspPoItemReceivingsByVendor.h"

class dspPoItemReceivingsByVendor : public display, public Ui::dspPoItemReceivingsByVendor
{
    Q_OBJECT

public:
    dspPoItemReceivingsByVendor(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = Qt::Window);

    virtual bool setParams(ParameterList&);

public slots:
    virtual void sCorrectReceiving();
    virtual void sCreateVoucher();
    virtual void sHandleVariance( bool pShowVariances );
    virtual void sMarkAsInvoiced();
    virtual void sPopulateMenu(QMenu*, QTreeWidgetItem*, int);

protected slots:
    virtual void languageChange();

private:
    virtual bool recvHasValue();
};

#endif // DSPPOITEMRECEIVINGSBYVENDOR_H
