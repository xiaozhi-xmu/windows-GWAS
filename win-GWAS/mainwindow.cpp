#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "workdirectory.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowTitle("AquaGWAS");
    // Intiate Icon.(cross icon)
    ui->pheFileToolButton->setIcon(QIcon(":/new/icon/images/plus.png"));
    ui->genoFileToolButton->setIcon(QIcon(":/new/icon/images/plus.png"));
    ui->mapFileToolButton->setIcon(QIcon(":/new/icon/images/plus.png"));
    ui->covarFileToolButton->setIcon(QIcon(":/new/icon/images/plus.png"));
    ui->kinFileToolButton->setIcon(QIcon(":/new/icon/images/plus.png"));

    // Press Ctrl to select more.
    ui->selectedPhenoListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->excludedPhenoListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

    ui->tabWidget->setCurrentIndex(0);
    // Recommend maf and  ms
    ui->mafDoubleSpinBox->setValue(0.05);
    ui->mindDoubleSpinBox->setValue(0.20);
    ui->genoDoubleSpinBox->setValue(0.05);

    // FID complete controler
    ui->fidFileLineEdit->setEnabled(false);
    ui->fidFileBrowButton->setEnabled(false);
//    ui->fidWarnLabel->setHidden(true);

    // Chr filter
    ui->filterChrFileLineEdit->setEnabled(false);
    ui->filterChrFileBrowButton->setEnabled(false);

    // Initiate variables.
    fileReader = new FileReader;
    workDirectory = new WorkDirectory;
    phenoSelector = new PhenoSelector;
    runningMsgWidget = new RunningMsgWidget;
    gemmaParamWidget = new GemmaParamWidget;
    emmaxParamWidget = new EmmaxParamWidget;
    qualityControl = new QualityCtrlWidget;
    //    process = new QProcess;
    ldByFamGroupButton = new QButtonGroup;

    // Default output directory setting
    workDirectory->setProjectName("pro1");
#ifdef QT_NO_DEBUG
    workDirectory->setOutputDirectory(QDir::currentPath()+"/output/");
#else
    workDirectory->setOutputDirectory("/home/yingwang/Desktop/out/");
#endif
    ui->projectNameLineEdit->setText(workDirectory->getProjectName());
    ui->outdirLineEdit->setText(workDirectory->getOutputDirectory()+"/"+workDirectory->getProjectName());

    // LD by family.
    ldByFamGroupButton->addButton(ui->yesLDByFamRadioButton);
    ldByFamGroupButton->addButton(ui->noLDByFamRadioButton);
    ldByFamGroupButton->setExclusive(true);

    connect(runningMsgWidget, SIGNAL(closeSignal()), this, SLOT(on_closeRunningWidget()));
    //generate cmd line
    connect(this->ui->pca_ld_cmdButton,SIGNAL(clicked()),this,SLOT(pca_ld_cmdButton_clicked()));

    connect(this->ui->annotation_cmdButton,SIGNAL(clicked()),this,SLOT(annotationCmdButton_clicked()));

    connect(this->ui->assoc_cmdButton,SIGNAL(clicked()),this,SLOT(cmdGWASButton_clicked()));
    // Multi thread to modify ui.
    connect(this, SIGNAL(runningMsgWidgetAppendText(QString)),
            this->runningMsgWidget, SLOT(on_appendText(QString)));
    connect(this, SIGNAL(runningMsgWidgetClearText()),
            this->runningMsgWidget, SLOT(on_clearText()));
    connect(this, SIGNAL(setLineEditTextSig(QLineEdit*, QString)),
            this, SLOT(on_setLineEditText(QLineEdit*, QString)));
    connect(this, SIGNAL(setButtonEnabledSig(QAbstractButton *, bool)),
            this, SLOT(on_setButtonEnabled(QAbstractButton *, bool)));
    connect(this, SIGNAL(setGraphViewerGraphSig(QStringList)),
            this, SLOT(on_setGraphViewerGraph(QStringList)));
    connect(this, SIGNAL(resetWindowSig()), this, SLOT(on_resetWindowSig()));
    connect(this, SIGNAL(setMsgBoxSig(const QString &, const QString &)),
            this, SLOT(on_setMsgBoxSig(const QString &, const QString &)));
    // connect MToolButton->rightClick
    connect(ui->pheFileToolButton, SIGNAL(closeFileSig()), this, SLOT(on_pheFileToolButton_closeFileSig()));
    connect(ui->genoFileToolButton, SIGNAL(closeFileSig()), this, SLOT(on_genoFileToolButton_closeFileSig()));
    connect(ui->mapFileToolButton, SIGNAL(closeFileSig()), this, SLOT(on_mapFileToolButton_closeFileSig()));
    connect(ui->kinFileToolButton, SIGNAL(closeFileSig()), this, SLOT(on_kinFileToolButton_closeFileSig()));
    connect(ui->covarFileToolButton, SIGNAL(closeFileSig()), this, SLOT(on_covarFileToolButton_closeFileSig()));


    // Add executable permission to the calling tool or script which need executable permission.
    addFilesExecutePermission(this->toolpath);
    addFilesExecutePermission(this->scriptpath+"annovar/");
    addFilesExecutePermission(this->scriptpath+"poplddecay/");
}

MainWindow::~MainWindow()
{
    // Free pointer.
    delete ui;
    delete fileReader;
    delete workDirectory;
    delete phenoSelector;
    runningMsgWidget->close();
    delete runningMsgWidget;
    gemmaParamWidget->close();
    delete gemmaParamWidget;
    emmaxParamWidget->close();
    delete emmaxParamWidget;
    qualityControl->close();
    delete qualityControl;
    delete ldByFamGroupButton;

    //    if (process)    // QProcess
    //    {
    //        process->terminate();
    //        process->waitForFinished(-1);
    //    }
    //    delete process;


}
//generate cmd line(function)
void MainWindow::cmdGWASButton_clicked()
{
    if (this->runningFlag)
    {
        QMessageBox::information(nullptr, "Error", "A project is running now.");
        return;
    }

    QString tool = ui->toolComboBox->currentText();
    QString phenotype = this->fileReader->getPhenotypeFile().replace(' ', "\\ ");
    QString genotype = this->fileReader->getGenotypeFile().replace(' ', "\\ ");
    QString map = this->fileReader->getMapFile().replace(' ', "\\ ");
    QString covar = this->fileReader->getCovariateFile().replace(' ', "\\ ");
    QString kinship = this->fileReader->getKinshipFile().replace(' ', "\\ ");
    QString out = this->workDirectory->getOutputDirectory().replace(' ', "\\ ");  // Include project name.
    QString name = this->workDirectory->getProjectName();

    if (genotype.isNull())
    {
        QMessageBox::information(nullptr, "Error", "Plese select a genotype file!  ");
        return;
    }
    if (phenotype.isNull())
    {
        QMessageBox::information(nullptr, "Error", "Plese select a phenotype file!  ");
        return;
    }
    if (out.isNull() || name.isNull())
    {
        QMessageBox::information(nullptr, "Error", "Plese select a  correct work directory!  ");
        return;
    }

    this->runningFlag = true;
    //  ui->runGwasButton->setDisabled(true);
    qApp->processEvents();


    QString model = ui->modelComboBox->currentText();
    QString maf = ui->mafRadioButton->isChecked()? ui->mafDoubleSpinBox->text():nullptr;
    QString mind = ui->mindRadioButton->isChecked()? ui->mindDoubleSpinBox->text():nullptr;
    QString geno = ui->genoRadioButton->isChecked()? ui->genoDoubleSpinBox->text():nullptr;
    //manhattan plot
    QString gwBase =  ui->gwBaseLineEdit->text();
    QString gwExpo = ui->gwExpoLineEdit->text();
    QString sgBase = ui->sgBaseLineEdit->text();
    QString sgExpo = ui->sgExpoLineEdit->text();

    QString gw =gwBase+'e'+gwExpo;
    QString sg =sgBase+'e'+sgExpo;
    QString cmdlist;

    if(tool=="gemma")
    {
        QMap<QString, QString> moreParam = this->gemmaParamWidget->getCurrentParam();
        //  QString cmdlist;
        //correct p-value
        QString correctionType = this->gemmaParamWidget->getCorrectionType();
        if(correctionType=="")
        {
            correctionType="no";
        }

        cmdlist.append("-A -T "+tool+" -M "+model+" --name "+name+" -p "+phenotype+" --mkinmat "+moreParam["makekin"]
                +" -g "+genotype+" -o "+out+" --kinmat "+moreParam["kinmatrix"]+" --correct "+correctionType);
        if(model=="LMM")
        {
            cmdlist.append(" --lmmtest " +moreParam["lmmtest"]);
        }
        if(model=="BSLMM")
        {
            cmdlist.append(" --bslmmmodel " +moreParam["bslmmmodel"]);
        }
        if(!kinship.isNull())
        {
            cmdlist.append(" -k "+kinship);
        }
        if (!maf.isNull())
        {
            cmdlist.append(" --maf "+maf);
        }
        if (!mind.isNull())
        {

            cmdlist.append(" --mind "+mind);
        }
        if (!geno.isNull())
        {

            cmdlist.append(" --geno "+geno);
        }




    }
    if(tool=="plink")
    {
        //  QString cmdlist;
        cmdlist.append("-A -T "+tool+" -M "+model+" --name "+name
                       +" -p "+phenotype
                       +" -g "+genotype+" -o "+out);
        if (!maf.isNull())
        {
            cmdlist.append(" --maf "+maf);
        }
        if (!mind.isNull())
        {

            cmdlist.append(" --mind "+mind);
        }
        if (!geno.isNull())
        {

            cmdlist.append(" --geno "+geno);
        }

        // emit runningMsgWidgetAppendText(cmdlist);


    }
    if(tool=="emmax")
    {

        QMap<QString, QString> moreParam = this->emmaxParamWidget->getCurrentParam();
        //correct p-value
        QString correctionType = this->emmaxParamWidget->getCorrectionType();
        if(correctionType=="")
        {
            correctionType="no";
        }
        //  QString cmdlist;

        cmdlist.append("-A -T "+tool+" --name "+name+" -M "+model+" -p "+phenotype
                       +" -g "+genotype+" -o "+out+" --mkinmat "+moreParam["makekin"]+" --kinmat "+moreParam["kinmatrix"]
                +" --correct "+correctionType);
        if(!kinship.isNull())
        {
            cmdlist.append(" -k "+kinship);
        }
        if (!maf.isNull())
        {
            cmdlist.append(" --maf "+maf);
        }
        if (!mind.isNull())
        {

            cmdlist.append(" --mind "+mind);
        }
        if (!geno.isNull())
        {

            cmdlist.append(" --geno "+geno);
        }



        // emit runningMsgWidgetAppendText(cmdlist);

    }
    cmdlist.append(" --gw "+gw+" --sg "+sg);
    if (qualityControl->isLinkageFilterNeeded())//如果QC小窗选了yes，则要产生相应的命令
    {
        QString winSize, stepLen, r2Threshold;
        this->qualityControl->getLinkageFilterType(winSize, stepLen, r2Threshold);
      //  cmdlist.append(" --qualityControl_SNPlinkage");
        cmdlist.append(" --winsize "+winSize+" --slen "+stepLen+" --r2th "+r2Threshold);
    }
    emit runningMsgWidgetClearText();
    emit runningMsgWidgetAppendText(cmdlist);
    // -A -T emmax --name pro2 -M EMMA -p /home/zhi/Desktop/data_renhao/sex.phe -g /home/zhi/Desktop/data_renhao/y2.tped


    this->runningFlag = false;
}

void MainWindow::pca_ld_cmdButton_clicked()//当点击pca/ld界面的cmdgenerate按键后，要产生对应的命令
{

    if (this->runningFlag)
    {
        QMessageBox::information(nullptr, "Error", "A project is running now.");
        return;
    }
    if (this->fileReader->getGenotypeFile().isNull() || this->fileReader->getGenotypeFile().isEmpty())
    {
        QMessageBox::information(nullptr, "Error", "A genotype file is necessary!   ");
        return;
    }

    this->runningFlag = true;
    //    ui->pcaRunPushButton->setEnabled(false);
    qApp->processEvents();

    QString genotype = this->fileReader->getGenotypeFile().replace(' ', "\\ ");;
    QString map = this->fileReader->getMapFile().replace(' ', "\\ ");;
    QString out = this->workDirectory->getOutputDirectory().replace(' ', "\\ ");;
    QString name = this->workDirectory->getProjectName().replace(' ', "\\ ");;
    QString PCs =QString(ui->nPCsLineEdit->text());
    QString Threads= QString(ui->nThreadsLineEdit->text());
    QString maf = ui->mafRadioButton->isChecked()? ui->mafDoubleSpinBox->text():nullptr;
    QString mind = ui->mindRadioButton->isChecked()? ui->mindDoubleSpinBox->text():nullptr;
    QString geno = ui->genoRadioButton->isChecked()? ui->genoDoubleSpinBox->text():nullptr;

    QString PCAcmdlist;
    PCAcmdlist.append("--pca ");
    PCAcmdlist.append("--name "+name);
    PCAcmdlist.append(" -g "+genotype+" --pcs "+PCs+" --threads "+Threads+" -o "+out);
    if (qualityControl->isLinkageFilterNeeded())//如果QC小窗选了yes，则要产生相应的命令
    {
        QString winSize, stepLen, r2Threshold;
        this->qualityControl->getLinkageFilterType(winSize, stepLen, r2Threshold);
      //  PCAcmdlist.append(" --qualityControl_SNPlinkage");
        PCAcmdlist.append(" --winsize "+winSize+" --slen "+stepLen+" --r2th "+r2Threshold);
    }
    //检查filterchr框是否被选中
    bool filterChrFlag = ui->filterChrRadioButton->isChecked();
    if (filterChrFlag)
    {
        QString listForChr = ui->filterChrFileLineEdit->text();
        PCAcmdlist.append(" --filchr "+listForChr);
    }
    //检查fidcomplete框是否被选中
    bool completeFidFlag = ui->compleFIDRadioButton->isChecked();
    if (completeFidFlag)
    {
        QString fidFile = ui->fidFileLineEdit->text();
        PCAcmdlist.append(" --fidcom "+fidFile);
    }


    QString LDcmdlist;
    LDcmdlist.append("--LD ");
    LDcmdlist.append("-g "+genotype+" --name "+name+" -o "+out+" --ldplot yes ");
    if(ui->yesLDByFamRadioButton->isChecked())
    {
        LDcmdlist.append("--analysis yes");
    }
    else
    {
        LDcmdlist.append("--analysis no");
    }

    if (!maf.isNull())
    {
        PCAcmdlist.append(" --maf " + maf);
        LDcmdlist.append(" --maf " + maf);
    }
    if (!mind.isNull())
    {
        PCAcmdlist.append(" --mind " + mind);
        LDcmdlist.append(" --mind " + mind);
    }
    if (!geno.isNull())
    {
        PCAcmdlist.append(" --geno " + geno);
        LDcmdlist.append(" --geno " + geno);
    }
    if (qualityControl->isLinkageFilterNeeded())//如果QC小窗选了yes，则要产生相应的命令
    {
        QString winSize, stepLen, r2Threshold;
        this->qualityControl->getLinkageFilterType(winSize, stepLen, r2Threshold);
      //  LDcmdlist.append(" --qualityControl_SNPlinkage");
        LDcmdlist.append(" --winsize "+winSize+" --slen "+stepLen+" --r2th "+r2Threshold);
    }
    //检查filterchr框是否被选中
    filterChrFlag = ui->filterChrRadioButton->isChecked();
    if (filterChrFlag)
    {
        QString listForChr = ui->filterChrFileLineEdit->text();
        LDcmdlist.append(" --filchr "+listForChr);
    }
    //检查fidcomplete框是否被选中
    completeFidFlag = ui->compleFIDRadioButton->isChecked();
    if (completeFidFlag)
    {
        QString fidFile = ui->fidFileLineEdit->text();
        LDcmdlist.append(" --fidcom "+fidFile);
    }


    emit runningMsgWidgetClearText();
    emit runningMsgWidgetAppendText("PCA:");
    emit runningMsgWidgetAppendText(PCAcmdlist);
    emit runningMsgWidgetAppendText("");
    emit runningMsgWidgetAppendText("");
    emit runningMsgWidgetAppendText("");
    emit runningMsgWidgetAppendText("");
    emit runningMsgWidgetAppendText("LD:");
    emit runningMsgWidgetAppendText(LDcmdlist);

    this->runningFlag = false;
    /*   LD:
         single:
         --LD -g /home/zhi/Desktop/data/hapmap1.vcf --analysis no --name pro1 -o /home/zhi/Desktop/out --LDplot yes
     family:
     --LD -g
     /home/zhi/Desktop/LD_by_family测试数据/222_filter1_124.ped --analysis yes --name pro2 -o /home/zhi/Desktop/out --LDplot yes*/
}

void MainWindow::annotationCmdButton_clicked()
{

    QString cmdlist;

    QString name = workDirectory->getProjectName().replace(' ', "\\ ");;
    QString out = workDirectory->getOutputDirectory().replace(' ', "\\ ");;
    QString vcfFile = this->fileReader->getGenotypeFile().replace(' ', "\\ ");;
    QString pvalFile = ui->annoPvalLineEdit->text().replace(' ', "\\ ");;    // p-value file(the first column is SNP_ID and the last column is p-value)
    QString avinputFilePath = ui->avinFileLineEdit->text().replace(' ', "\\ ");;//先读文件,若要step则后面会覆盖
    QString refGeneFilePath = ui->refGeneFileLineEdit->text().replace(' ', "\\ ");;
    QString refSeqFilePath = ui->refSeqFileLineEdit->text().replace(' ', "\\ ");;
    QString snpPosFilePath = ui->snpPosFileLineEdit->text().replace(' ', "\\ ");;//先读文件,若要step则后面会覆盖
    QString funcAnnoRefFilePath = ui->funcAnnoRefFileLineEdit->text().replace(' ', "\\ ");;

    //step cmd
    if(checkoutExistence(pvalFile)&& !(checkoutExistence(vcfFile)))//如果只输入p-value文件，而没有基因型文件，则报错
    {
        emit setMsgBoxSig("Error", "if you want to step .vcf file is necessary! ");
        return;
    }
    bool step_flag=0;
    if(checkoutExistence(pvalFile)&&checkoutExistence(vcfFile))//两个文件都有输入说明要做step
    {
        step_flag=1;
        QString thBase = ui->annoThBaseLineEdit->text();    // Threshold base number.
        QString thExpo = ui->annoThExpoLineEdit->text();    // Threshold exponent.
        QString threshold = thBase+'e'+thExpo; //如1e-5


        QFileInfo vcfFileInfo(vcfFile);
        QString vcfFileAbPath = vcfFileInfo.absolutePath();
        QString vcfFileBaseName = vcfFileInfo.baseName();
        avinputFilePath = vcfFileAbPath + "/" + vcfFileBaseName + ".avinput";   // For input of structural annotaion
        snpPosFilePath = vcfFileAbPath + "/" + vcfFileBaseName + "_SNPpos";     // For input of functional annotation
        cmdlist.append("--step");
        cmdlist.append(" --pval "+pvalFile+" -g "+vcfFile+" --thre "+threshold);
    }


    qApp->processEvents();
    //structural anno
    if (avinputFilePath.isNull() || avinputFilePath.isEmpty())
    {
        emit setMsgBoxSig("Error", ".avinput file is necessary! ");
        return;
        //   throw -1;
    }
    if (refGeneFilePath.isNull() || refGeneFilePath.isEmpty())
    {
        emit setMsgBoxSig("Error", "if you want to do structural anno Gff or Gtf file is necessary! ");
        return;
        //  throw -1;
    }
    if (refSeqFilePath.isNull() || refSeqFilePath.isEmpty())
    {
        emit setMsgBoxSig("Error", "if you want to do structural anno Reference sequence file is necessary! ");
        return;
        // throw -1;
    }


    cmdlist.append(" --struanno");
    cmdlist.append(" --name "+name+" --refgene "+refGeneFilePath+" --refseq "+refSeqFilePath
                   +" -o "+out);
    if(step_flag==0)//如果没有做step，才输出avin命令，如果有做step，就不输出avin命令，因为cmd代码里会自动生成avin结果的路径
    {
        cmdlist.append(" --avin "+avinputFilePath);
    }


    QFileInfo fileInfo(refGeneFilePath);
    QString baseName = fileInfo.baseName();
    QString outFilePath = out + "/" + name + "_" + baseName;//此为structural anno的结果路径,其中两个结果作为fun anno的输入
    QString varFuncFilePath = outFilePath+".variant_function";
    QString exVarFuncFilePath = outFilePath+".exonic_variant_function";

    if ((!(funcAnnoRefFilePath.isNull()||funcAnnoRefFilePath.isEmpty())) )//如果用户输入了funcannoRef文件，说明想做funanno，则检查是否有snp_pos文件
    {
        if(snpPosFilePath.isNull()||snpPosFilePath.isEmpty())
        {
            emit setMsgBoxSig("Error", "if you want to do functional anno, SNP position file is necessary! ");

            return;
        }

    }

    if ((!(snpPosFilePath.isNull()||snpPosFilePath.isEmpty())) &&
            (!(funcAnnoRefFilePath.isNull()||funcAnnoRefFilePath.isEmpty())) &&
            (!(varFuncFilePath.isNull()||varFuncFilePath.isNull())) &&
            (!(exVarFuncFilePath.isNull()||exVarFuncFilePath.isEmpty())))//如果有这4个文件说明要做functional anno
    {

        cmdlist.append(" --funcanno "+funcAnnoRefFilePath);
                   //    +" --var "+varFuncFilePath+" --exvar "+exVarFuncFilePath)
        if(step_flag==0)//如果没有做step，才输出snppos命令，如果有做step，就不输出snppos命令，因为cmd代码里会自动生成snppos结果的路径
        {
            cmdlist.append(" --snppos "+snpPosFilePath);
        }
    }
    /*    --funcAnno -o /home/zhi/Desktop/out --name pro1
     --snp_pos /home/zhi/Desktop/data_renhao/fan_sig_pos
     --funcAnnoRef /home/zhi/Desktop/Func_anno_ref/Hdhv3_changeID_annotation.ensem.csv
     --var /home/zhi/Desktop/out/pro2_Hdhv3.variant_function
     --exvar /home/zhi/Desktop/out/pro2_Hdhv3.exonic_variant_function */
    emit runningMsgWidgetClearText();
    emit runningMsgWidgetAppendText(cmdlist);

}

/**
 * @brief MainWindow::on_pheFileToolButton_clicked
 *          Open phenotype file
 */
void MainWindow::on_pheFileToolButton_clicked()
{
    // Basic display
    QFileDialog *fileDialog = new QFileDialog(this, "Open phenotype file", "", "pheno(*.phe *.txt);;all(*)");
    fileDialog->setViewMode(QFileDialog::Detail);

    QStringList fileNames;  // Absolute directory of file.
    if (fileDialog->exec())
    {
        fileNames = fileDialog->selectedFiles();
    }
    delete fileDialog;
    if (fileNames.isEmpty())    // If didn't choose any file.
    {
        return;
    }

    QFile fptr(fileNames[0]);
    fptr.open(QIODevice::ReadOnly|QIODevice::Text);
    QString phenoFirstLine = fptr.readLine();
    phenoFirstLine.replace("\r\n", "");         // Strip "\n"
    phenoFirstLine.replace("\n", "");
    QStringList phenoList = phenoFirstLine.split(QRegExp("\\s+"), QString::SkipEmptyParts);;

    if (phenoList.length() < 3)
    {   // Basic cotent: FID IIF PHE.
        QMessageBox::information(nullptr, "Error", "Phenotype file format error!    ");
        ui->selectedPhenoListWidget->clear();
        ui->excludedPhenoListWidget->clear();
        return;
    }

    ui->pheFileToolButton->setShowMenuFlag(true);
    ui->pheFileToolButton->setIcon(QIcon(":/new/icon/images/file.png"));    // Set file Icon.
    this->fileReader->setPhenotypeFile(fileNames[0]);

    QFileInfo  pheFileInfo(fileNames[0]);
    QString fileBaseName = pheFileInfo.baseName();
    QString fileName = pheFileInfo.fileName(); // Get the file name from a path.
    ui->pheFileLabel->setText(fileName);

    // Get types of phenotype, and write to list widget.
    QString fileSuffix = pheFileInfo.suffix();
    if (fileSuffix != "phe")
    {   // FID IID PHE1 PHE2 PHE3 ... (With header)

        phenoList.removeFirst();    // Remove first two columns
        phenoList.removeFirst();
        phenoSelector->setSelectedPheno(phenoList);
    }
    else
    {   // FID IID PHE (No header)
        QStringList phenoList;
        phenoList.append(fileBaseName);
        phenoSelector->setSelectedPheno(phenoList);
    }

    QStringList list;
    this->phenoSelector->setExcludedPheno(list);

    ui->selectedPhenoListWidget->clear();
    ui->excludedPhenoListWidget->clear();
    ui->selectedPhenoListWidget->insertItems(0, phenoSelector->getSelectedPheno());
}

/**
 * @brief MainWindow::on_pheFileToolButton_closeFileSig
 *          Close phenotype file
 */
void MainWindow::on_pheFileToolButton_closeFileSig()
{
    ui->pheFileToolButton->setShowMenuFlag(false);
    ui->pheFileToolButton->setIcon(QIcon(":/new/icon/images/plus.png"));    // Set plus Icon.
    ui->pheFileLabel->setText("empty");
    ui->selectedPhenoListWidget->clear();
    ui->excludedPhenoListWidget->clear();
    this->fileReader->setPhenotypeFile(nullptr);
}

void MainWindow::on_genoFileToolButton_clicked()
{
    QFileDialog *fileDialog = new QFileDialog(this, "Open genotype file", "", "geno(*.vcf *.ped *.tped *.bed);;all(*)");
    fileDialog->setViewMode(QFileDialog::Detail);

    QStringList fileNames;
    if (fileDialog->exec())
    {
        fileNames = fileDialog->selectedFiles();
    }
    delete fileDialog;
    if (fileNames.isEmpty())    // If didn't choose any file.
    {
        return;
    }
    ui->genoFileToolButton->setShowMenuFlag(true);
    ui->genoFileToolButton->setIcon(QIcon(":/new/icon/images/file.png"));
    this->fileReader->setGenotypeFile(fileNames[0]);

    QFileInfo  genoFileInfo(fileNames[0]);
    QString fileName = genoFileInfo.fileName(); // Get the file name from a path.
    ui->genoFileLabel->setText(fileName);
}

void MainWindow::on_genoFileToolButton_closeFileSig()
{
    ui->genoFileToolButton->setShowMenuFlag(false);
    ui->genoFileToolButton->setIcon(QIcon(":/new/icon/images/plus.png"));    // Set plus Icon.
    ui->genoFileLabel->setText("empty");
    this->fileReader->setGenotypeFile(nullptr);
}

void MainWindow::on_mapFileToolButton_clicked()
{
    QFileDialog *fileDialog = new QFileDialog(this, "Open map file", "", "map(*.map *.tfam);;all(*)");
    fileDialog->setViewMode(QFileDialog::Detail);

    QStringList fileNames;
    if (fileDialog->exec())
    {
        fileNames = fileDialog->selectedFiles();
    }
    delete fileDialog;
    if (fileNames.isEmpty())    // If didn't choose any file.
    {
        return;
    }
    ui->mapFileToolButton->setShowMenuFlag(true);
    ui->mapFileToolButton->setIcon(QIcon(":/new/icon/images/file.png"));
    this->fileReader->setMapFile(fileNames[0]);

    QFileInfo  mapFileInfo(fileNames[0]);
    QString fileName = mapFileInfo.fileName(); // Get the file name from a path.
    ui->mapFileLabel->setText(fileName);
}

void MainWindow::on_mapFileToolButton_closeFileSig()
{
    ui->mapFileToolButton->setShowMenuFlag(false);
    ui->mapFileToolButton->setIcon(QIcon(":/new/icon/images/plus.png"));    // Set plus Icon.
    ui->mapFileLabel->setText("empty");
    this->fileReader->setMapFile(nullptr);
}

void MainWindow::on_covarFileToolButton_clicked()
{
    QFileDialog *fileDialog = new QFileDialog(this, "Open covariate file", "", "covar(*.eigenvec *eigenval *.cov *.covar *.txt);;all(*)");
    fileDialog->setViewMode(QFileDialog::Detail);

    QStringList fileNames;
    if (fileDialog->exec())
    {
        fileNames = fileDialog->selectedFiles();
    }
    delete fileDialog;
    if (fileNames.isEmpty())    // If didn't choose any file.
    {
        return;
    }
    ui->covarFileToolButton->setShowMenuFlag(true);
    ui->covarFileToolButton->setIcon(QIcon(":/new/icon/images/file.png"));
    this->fileReader->setCovariateFile(fileNames[0]);

    QFileInfo  covarFileInfo(fileNames[0]);
    QString fileName = covarFileInfo.fileName(); // Get the file name from a path.
    ui->covarFileLabel->setText(fileName);
}

void MainWindow::on_covarFileToolButton_closeFileSig()
{
    ui->covarFileToolButton->setShowMenuFlag(false);
    ui->covarFileToolButton->setIcon(QIcon(":/new/icon/images/plus.png"));    // Set plus Icon.
    ui->covarFileLabel->setText("empty");
    this->fileReader->setCovariateFile(nullptr);
}

void MainWindow::on_kinFileToolButton_clicked()
{
    QFileDialog *fileDialog = new QFileDialog(this, "Open kinship file", "", "kin(*.kin *.kinf *.txt);;all(*)");
    fileDialog->setViewMode(QFileDialog::Detail);

    QStringList fileNames;
    if (fileDialog->exec())
    {
        fileNames = fileDialog->selectedFiles();
    }
    delete fileDialog;
    if (fileNames.isEmpty())    // If didn't choose any file.
    {
        return;
    }
    ui->kinFileToolButton->setShowMenuFlag(true);
    ui->kinFileToolButton->setIcon(QIcon(":/new/icon/images/file.png"));
    this->fileReader->setKinshipFile(fileNames[0]);

    QFileInfo  kinFileInfo(fileNames[0]);
    QString fileName = kinFileInfo.fileName(); // Get the file name from a path.
    ui->kinFileLabel->setText(fileName);
}

void MainWindow::on_kinFileToolButton_closeFileSig()
{
    ui->kinFileToolButton->setShowMenuFlag(false);
    ui->kinFileToolButton->setIcon(QIcon(":/new/icon/images/plus.png"));    // Set plus Icon.
    ui->kinFileLabel->setText("empty");
    this->fileReader->setKinshipFile(nullptr);
}

/**
 * @brief MainWindow::on_outdirBrowButton_clicked
 *          Choose output directory.
 */
void MainWindow::on_outdirBrowButton_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Choose directory");
    if (!dir.isEmpty())
    {
        this->workDirectory->setOutputDirectory(dir);
        ui->outdirLineEdit->setText(dir+"/"+this->workDirectory->getProjectName());
    }
}


void MainWindow::on_excludeAllPhenoButton_clicked()
{
    if (ui->selectedPhenoListWidget->count() <= 1)
    {
        return;
    }
    phenoSelector->excludeAllPheno();
    ui->selectedPhenoListWidget->clear();   // Clear the list widget.
    ui->excludedPhenoListWidget->clear();
    ui->selectedPhenoListWidget->insertItems(0, phenoSelector->getSelectedPheno()); // Display
    ui->excludedPhenoListWidget->insertItems(0, phenoSelector->getExcludedPheno());
}

void MainWindow::on_selectAllPhenoButton_clicked()
{
    phenoSelector->selectAllPheno();
    ui->selectedPhenoListWidget->clear();
    ui->excludedPhenoListWidget->clear();
    ui->selectedPhenoListWidget->insertItems(0, phenoSelector->getSelectedPheno());
    ui->excludedPhenoListWidget->insertItems(0, phenoSelector->getExcludedPheno());
}

void MainWindow::on_selectPhenoButton_clicked()
{
    phenoSelector->selectPheno(ui->excludedPhenoListWidget->selectedItems());
    ui->selectedPhenoListWidget->clear();
    ui->excludedPhenoListWidget->clear();
    ui->selectedPhenoListWidget->insertItems(0, phenoSelector->getSelectedPheno());
    ui->excludedPhenoListWidget->insertItems(0, phenoSelector->getExcludedPheno());
}

void MainWindow::on_excludePhenoButton_clicked()
{
    if (ui->selectedPhenoListWidget->count() <= 1)
    {
        return;
    }
    phenoSelector->excludePheno(ui->selectedPhenoListWidget->selectedItems());
    ui->selectedPhenoListWidget->clear();
    ui->excludedPhenoListWidget->clear();
    ui->selectedPhenoListWidget->insertItems(0, phenoSelector->getSelectedPheno());
    ui->excludedPhenoListWidget->insertItems(0, phenoSelector->getExcludedPheno());
}

/**
 * @brief MainWindow::on_rungwasButton_clicked
 *          Run GWAS
 */
void MainWindow::on_runGwasButton_clicked()
{
    if (this->runningFlag)
    {
        QMessageBox::information(nullptr, "Error", "A project is running now.");
        return;
    }

    QString tool = ui->toolComboBox->currentText();
    QString phenotype = this->fileReader->getPhenotypeFile();
    QString genotype = this->fileReader->getGenotypeFile();
    QString map = this->fileReader->getMapFile();
    QString covar = this->fileReader->getCovariateFile();
    QString kinship = this->fileReader->getKinshipFile();
    QString out = this->workDirectory->getOutputDirectory();  // Include project name.
    QString name = this->workDirectory->getProjectName();

    if (phenotype.isNull())
    {
        QMessageBox::information(nullptr, "Error", "Plese select a phenotype file!  ");
        return;
    }
    if (genotype.isNull())
    {
        QMessageBox::information(nullptr, "Error", "Plese select a genotype file!  ");
        return;
    }
    if (out.isNull() || name.isNull())
    {
        QMessageBox::information(nullptr, "Error", "Plese select a  correct work directory!  ");
        return;
    }

    this->runningFlag = true;
    ui->runGwasButton->setDisabled(true);
    qApp->processEvents();

    QFileInfo pheFileInfo(phenotype);
    QString pheFileBaseName = pheFileInfo.baseName();
    QString pheFileAbPath = pheFileInfo.absolutePath();
    QString pheFileSuffix = pheFileInfo.suffix();

    QFuture<void> fu = QtConcurrent::run(QThreadPool::globalInstance(), [&]()
    {
        if (pheFileSuffix == "phe")
        {   // Only one phenotype data.
            if (tool == "emmax")
            {
                if (!this->callEmmaxGwas(phenotype, genotype, map, covar, kinship, out, name))
                {
                    emit resetWindowSig();
                    QThread::msleep(10);
                    return;
                }
            }

            if (tool == "gemma")
            {
                if (!this->callGemmaGwas(phenotype, genotype, map, covar, kinship, out, name))
                {
                    emit resetWindowSig();
                    QThread::msleep(10);
                    return;
                }
            }

            if (tool == "plink")  // plink GWAS
            {
                if (!this->callPlinkGwas(phenotype, genotype, map, covar, kinship, out, name))
                {
                    emit resetWindowSig();
                    QThread::msleep(10);
                    return;
                }
            }
        }
        else
        {   // There several phenotype data.
            for (int i = 0; i < ui->selectedPhenoListWidget->count(); i++)
            {   // Make .phe file then run GWAS one by one.
                QListWidgetItem *item = ui->selectedPhenoListWidget->item(i);

                if (!this->makePheFile(phenotype, item->text()))
                {
                    emit resetWindowSig();
                    QThread::msleep(10);
                    return;
                }
                QString madedPheFile = pheFileAbPath + "/" + item->text() + ".phe";
                if (tool == "emmax")
                {
                    if (!this->callEmmaxGwas(madedPheFile, genotype, map, covar, kinship, out, name))
                    {
                        emit resetWindowSig();
                        QThread::msleep(10);

                        return;
                    }
                }

                if (tool == "gemma")
                {
                    if (!this->callGemmaGwas(madedPheFile, genotype, map, covar, kinship, out, name))
                    {
                        emit resetWindowSig();
                        QThread::msleep(10);
                        return;
                    }
                }

                if (tool == "plink")  // plink GWAS
                {
                    if (!this->callPlinkGwas(madedPheFile, genotype, map, covar, kinship, out, name))
                    {
                        emit resetWindowSig();
                        QThread::msleep(10);
                        return;
                    }
                }
            }
        }
    });
    while (!fu.isFinished())
    {
        qApp->processEvents(QEventLoop::AllEvents, 50);
        QThread::msleep(10);
    }

    this->resetWindow();
    this->runningFlag = false;
}

bool MainWindow::drawManAndQQ(QString inputFile)
{
    if (!checkoutExistence(inputFile))
    {
        return false;
    }
    QString gwasResulFile = inputFile;
    if (gwasResulFile.isEmpty())
    {   // Gwas result file is necessary.
        emit runningMsgWidgetAppendText(QDateTime::currentDateTime().toString() +
                                        "\nA GWAS result file is necessary.\n");
        return false;
    }

    emit runningMsgWidgetAppendText(QDateTime::currentDateTime().toString() +
                                    "\nMake qqman input file, \n");
    QThread::msleep(10);
    // Transform gwas result file type to input file type of qqman.
    QStringList qqmanFile = makeQQManInputFile(gwasResulFile); //   path/name.gemma_wald
    QStringList manOutList, qqOutList;
    if (qqmanFile.isEmpty())
    {   // makeQQManInputFile error.
        emit runningMsgWidgetAppendText(QDateTime::currentDateTime().toString() +
                                        "\nMake qqman input file ERROR.\n");
        QThread::msleep(10);
        return false;
    }
    emit runningMsgWidgetAppendText(QDateTime::currentDateTime().toString() +
                                    "\nMake qqman input file OK.\n");
    QThread::msleep(10);

    for (auto item:qqmanFile)
    {   // Multiple result, multiple output plot, append to list.
        manOutList.append(this->workDirectory->getOutputDirectory()+"/"+this->workDirectory->getProjectName()
                       +"_"+item.split(".")[item.split(".").length()-1]+"_man.png");
    }

    emit runningMsgWidgetAppendText(QDateTime::currentDateTime().toString() +
                                    "\nDraw manhattan plot, \n");
    QThread::msleep(10);
    if (!this->drawManhattan(qqmanFile, manOutList))
    {   // drawManhattan error
        emit runningMsgWidgetAppendText(QDateTime::currentDateTime().toString() +
                                        "\nDraw manhattan plot ERROR.\n");
        QThread::msleep(10);
        throw -1;
    }

    emit runningMsgWidgetAppendText(QDateTime::currentDateTime().toString() +
                                    "\nDraw manhattan plot OK." +
                                    "\nmanhattan plot: \n" +
                                    manOutList.join("\n")+"\n");
    QThread::msleep(10);

    for (auto item:qqmanFile)
    {   // Multiple result, multiple output plot.
        qqOutList.append(this->workDirectory->getOutputDirectory()+"/"+this->workDirectory->getProjectName()
                       +"_"+item.split(".")[item.split(".").length()-1]+"_qq.png");
    }

    emit runningMsgWidgetAppendText(QDateTime::currentDateTime().toString() +
                                    "\nDraw QQ plot, \n");
    QThread::msleep(10);
    if (!this->drawQQplot(qqmanFile, qqOutList))
    {
        emit runningMsgWidgetAppendText(QDateTime::currentDateTime().toString() +
                                        "\nDraw QQ plot ERROR. \n");
        QThread::msleep(10);
        throw -1;
    }

    emit runningMsgWidgetAppendText(QDateTime::currentDateTime().toString() +
                                    "\nDraw QQ plot OK. " +
                                    "\nQQ plot: \n" + qqOutList.join("\n") + "\n");
    QThread::msleep(10);

    QFile file;
    for (auto item:qqmanFile)
    {   // Remove intermediate file.
        if (item == gwasResulFile)
        {
            continue;
        }
        file.remove(item);
    }

//     Show plot
    if (this->runningFlag && checkoutExistence(manOutList[0]))
    {
        emit setGraphViewerGraphSig(manOutList+qqOutList);
        QThread::msleep(10);
    }

    return true;
}

/**
 * @brief MainWindow::callGemmaGwas
 *      Call gemma to GWAS(Whole process of gemma are implemeted here)
 * @param phenotype (FID MID PHE)
 * @param genotype
 * @param map       :will to find in the same path(and prefix) of genotype file.
 * @param covar
 * @param kinship
 * @param out
 * @param name
 * @return
 */
bool MainWindow::callGemmaGwas(QString phenotype, QString genotype, QString map,
                               QString covar, QString kinship, QString out, QString name)
{


    return true;
}

/**
 * @brief MainWindow::callEmmaxGwas
 *      Call emmax to GWAS(Whole process of emmax are implemeted here)
 * @param phenotype
 * @param genotype
 * @param map
 * @param covar
 * @param kinship
 * @param out
 * @param name
 * @return
 */
bool MainWindow::callEmmaxGwas(QString phenotype, QString genotype, QString map,
                               QString covar, QString kinship, QString out, QString name)
{


    return true;
}

/**
 * @brief MainWindow::callPlinkGwas
 *      Call gemma to GWAS(Whole process of gemma are implemeted here)
 * @param phenotype
 * @param genotype
 * @param map
 * @param covar
 * @param kinship
 * @param out
 * @param name
 * @return
 */
bool MainWindow::callPlinkGwas(QString phenotype, QString genotype, QString map,
                               QString covar, QString kinship, QString out, QString name)
{

    return true;
}

/**
 * @brief MainWindow::on_closeRunningWidget
 */
void MainWindow::on_closeRunningWidget()
{
    if (!this->runningMsgWidget->isVisible())
    {
        return;
    }

    if (this->runningFlag)
    {
        // Juage there are any tools running now.
        QMessageBox::StandardButton ret = QMessageBox::information(this, "WARNING",
                                                                   "The project may be terminated if close the widget!   ",
                                                                   QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

        if (ret == QMessageBox::Yes)
        {
            emit terminateProcess();
            this->runningFlag = false;
            this->runningMsgWidget->clearText();
            this->runningMsgWidget->hide();
            this->resetWindow();
        }
        return;
    }
    else
    {   // Close widget directly while no tool running.
        this->runningMsgWidget->clearText();
        this->runningMsgWidget->hide();
    }
}

/**
 * @brief MainWindow::isVcfFile
 *      Judge the file whether a VCF file from file name.
 * @param file
 * @return
 */
bool MainWindow::isVcfFile(QString file) // Just consider filename.
{
    if (file.isNull() || file.isEmpty())
    {
        return false;
    }

    QFileInfo fileInfo = QFileInfo(file);
    QStringList fileNameList = fileInfo.fileName().split(".");         // demo.vcf.gz

    for (QString item : fileNameList)
    {
        if (item == "vcf")
        {
            return true;
        }
    }

    return false;
}

/**
 * @brief MainWindow::resetWindow
 *      Reset MainWindow.
 */
void MainWindow::resetWindow()
{
    ui->runGwasButton->setEnabled(true);
    ui->ldRunPushButton->setEnabled(true);
    ui->pcaRunPushButton->setEnabled(true);
//    ui->drawManPushButton->setEnabled(true);
//    ui->drawQQPushButton->setEnabled(true);
    ui->annotationRunButton->setEnabled(true);
    ui->annoStepPushButton->setEnabled(true);
}

/**
 * @brief MainWindow::on_toolComboBox_currentTextChanged
 *      Select a new tool and get supported model of tool.
 * @param tool
 */
void MainWindow::on_toolComboBox_currentTextChanged(const QString &tool)
{
    ui->modelComboBox->clear();
    if (tool == "plink")
    {
        QStringList model ;
        model << "Linear" << "Logistic";
        ui->modelComboBox->addItems(model);

    }

    if (tool == "gemma")
    {
        QStringList model ;
        model << "LMM" << "BSLMM";
       // Gemma gemma;
        ui->modelComboBox->addItems(  model);
    }
    if (tool == "emmax")
    {
        QStringList model ;
        model << "EMMA";
      //  Emmax emmax;
        ui->modelComboBox->addItems(model);
    }
}

/**
 * @brief MainWindow::makePheFile
 *      Make phenotype file according selected phenotype.
 * @param phenotype     phenotype file path
 * @param selectedPheno
 * @return
 */
bool MainWindow::makePheFile(QString const phenotype, QString const selectedPheno)
{
    if (phenotype.isNull() || selectedPheno.isEmpty())
    {
        return false;
    }

    QFileInfo pheFileInfo(phenotype);
    QString pheFileAbPath = pheFileInfo.absolutePath();
    QString pheFileBaseName = pheFileInfo.baseName();

    QFile srcFile(phenotype);
    QFile dstFile(pheFileAbPath+"/"+selectedPheno+".phe");

    if (!srcFile.open(QIODevice::ReadOnly) || !dstFile.open(QIODevice::ReadWrite))
    {
        return false;
    }

    QTextStream srcFileStream(&srcFile);
    QTextStream dstFileStream(&dstFile);

    QStringList header = srcFileStream.readLine().split("\t");   // header

    int selectedPheIndex = header.indexOf(selectedPheno);   // get the columns of selected phenotype.

    if (selectedPheIndex == -1)
    {
        return false;
    }

    while (!srcFileStream.atEnd())
    {
        QString outLine;
        QStringList line = srcFileStream.readLine().split("\t", QString::SkipEmptyParts);
        if (selectedPheIndex >= line.length())
        {   // Any missing rate?
            outLine = line[0] + "\t" + line[1] + "\tNA\n";
        }
        else
        {
            outLine = line[0] + "\t" + line[1] + "\t" + line[selectedPheIndex] + "\n";
        }
        //dstFileStream << line[0] << "\t" << line[1] << "\t" << line[selectedPheIndex] << "\n";
        dstFileStream << outLine;
    }

    srcFile.close();
    dstFile.close();

    return true;
}

/**
 * @brief MainWindow::on_detailPushButton_clicked
 *      Set and show paramWidget
 */
void MainWindow::on_detailPushButton_clicked()
{
    if (ui->toolComboBox->currentText() == "gemma")
    {
        if (ui->modelComboBox->currentText() == "LMM")
        {
            this->gemmaParamWidget->setLmmParamEnabled(true);
            this->gemmaParamWidget->setBslmmParamEnabled(false);
        }
        else
        {
            this->gemmaParamWidget->setLmmParamEnabled(false);
            this->gemmaParamWidget->setBslmmParamEnabled(true);
        }

        this->gemmaParamWidget->show();
    }

    if (ui->toolComboBox->currentText() == "emmax")
    {
        this->emmaxParamWidget->show();
    }
}

/**
 * @brief MainWindow::refreshMessage
 *      To mananage running message.
 * @param curText
 * @param newText
 * @return
 */
QString MainWindow::refreshMessage(QString curText, QString newText)
{   // Remain lots of problems, little validance now.
    if (newText.isEmpty())
    {   // newMsg don't have valid message.
        return curText;
    }
    if (curText.isEmpty())
    {   // The first message.
        return newText;
    }

    QRegExp regRxp("\\s*((100|[1-9]?\\d)%\\s*)");

    // For current text: replace pecent number to null string.
    curText.replace(QRegExp("\\s*(100|[1-9]?\\d)%\\s*"), "");
    // For current text: repalce multiple line break to only one.
    curText.replace(QRegExp("\n\n+"), "\n");
    // For current text: replace multiple '=' to null string.
    //    (only matching gemma output message)
    curText.replace(QRegExp("=+"), "");

    // Only consider gemma here (gemma msg: ===========   12%)
    QRegExp gemmaMsg("(=+\\s*(100|[1-9]?\\d)%\\s*)+");
    int pos = gemmaMsg.indexIn(newText);
    if (pos > -1)
    {   // Will cause multiple '\n' in current text.
        // So it's necessary to repalce multiple line break to only one in current text.
        newText = "\n"+gemmaMsg.cap(1);
    }

    return curText + newText;
}


void MainWindow::on_projectNameLineEdit_textChanged(const QString &text)
{   // When edit finished and the current text is empty, set a default name.
    this->workDirectory->setProjectName(text);
    if (!ui->outdirLineEdit->text().isEmpty())
    {   // If a out directory is selected, display the out directory + the project name.
        ui->outdirLineEdit->setText(this->workDirectory->getOutputDirectory()+"/"+text);
    }
}

void MainWindow::on_projectNameLineEdit_editingFinished()
{   // Edit is finised but current text is empty.
    if (ui->projectNameLineEdit->text().isEmpty())
    {
        ui->projectNameLineEdit->setText("pro1");
    }
}

void MainWindow::on_outdirLineEdit_editingFinished()
{
    if (ui->outdirLineEdit->text().isEmpty())
    {
        ui->outdirLineEdit->setText(this->workDirectory->getOutputDirectory() + "/" +
                                    this->workDirectory->getProjectName());
    }
}

//void MainWindow::on_drawManPushButton_clicked()
//{
//    if (this->runningFlag)
//    {
//        QMessageBox::information(nullptr, "Error", "A project is running now.");
//        return;
//    }

//    this->runningFlag = true;
//    ui->drawManPushButton->setEnabled(false);
//    qApp->processEvents();

//    QFuture<void> fu = QtConcurrent::run(QThreadPool::globalInstance(), [&]()
//    {
//        try {
//            QString gwasResulFile = ui->qqmanGwasResultLineEdit->text();
//            if (gwasResulFile.isEmpty())
//            {   // Gwas result file is necessary.
//                emit runningMsgWidgetAppendText(QDateTime::currentDateTime().toString() +
//                                                "\nA GWAS result file is necessary.\n");
//                throw -1;
//            }

//            emit runningMsgWidgetAppendText(QDateTime::currentDateTime().toString() +
//                                            "\nMake qqman input file, \n");
//            QThread::msleep(10);
//            // Transform gwas result file type to input file type of qqman.
//            QStringList qqmanFile = makeQQManInputFile(gwasResulFile); //   path/name.gemma_wald
//            QStringList outList;
//            if (qqmanFile.isEmpty())
//            {   // makeQQManInputFile error.
//                emit runningMsgWidgetAppendText(QDateTime::currentDateTime().toString() +
//                                                "\nMake qqman input file ERROR.\n");
//                QThread::msleep(10);
//                throw -1;
//            }
//            emit runningMsgWidgetAppendText(QDateTime::currentDateTime().toString() +
//                                            "\nMake qqman input file OK.\n");
//            QThread::msleep(10);

//            for (auto item:qqmanFile)
//            {   // Multiple result, multiple output plot, append to list.
//                outList.append(this->workDirectory->getOutputDirectory()+"/"+this->workDirectory->getProjectName()
//                               +"_"+item.split(".")[item.split(".").length()-1]+"_man.png");
//            }

//            emit runningMsgWidgetAppendText(QDateTime::currentDateTime().toString() +
//                                            "\nDraw manhattan plot, \n");
//            QThread::msleep(10);
//            if (!this->drawManhattan(qqmanFile, outList))
//            {   // drawManhattan error
//                emit runningMsgWidgetAppendText(QDateTime::currentDateTime().toString() +
//                                                "\nDraw manhattan plot ERROR.\n");
//                QThread::msleep(10);
//                throw -1;
//            }

//            emit runningMsgWidgetAppendText(QDateTime::currentDateTime().toString() +
//                                            "\nDraw manhattan plot OK." +
//                                            "\nmanhattan plot: \n" +
//                                            outList.join("\n")+"\n");
//            QThread::msleep(10);

//            QFile file;
//            for (auto item:qqmanFile)
//            {   // Remove intermediate file.
//                if (item == gwasResulFile)
//                {
//                    continue;
//                }
//                file.remove(item);
//            }
//        } catch (...) {
//            emit resetWindowSig();
//            QThread::msleep(10);    // reset MainWindow
//        }
//    });
//    while (!fu.isFinished())
//    {
//        qApp->processEvents(QEventLoop::AllEvents, 200);
//    }
//    this->resetWindow();
//    this->runningFlag = false;
//}

//void MainWindow::on_drawQQPushButton_clicked()
//{
//    if (this->runningFlag)
//    {
//        QMessageBox::information(nullptr, "Error", "A project is running now.");
//        return;
//    }
//    this->runningFlag = true;
//    ui->drawQQPushButton->setEnabled(false);
//    qApp->processEvents();
//    QFuture<void> fu = QtConcurrent::run(QThreadPool::globalInstance(), [&]()
//    {
//        try {
//            QString gwasResulFile = ui->qqmanGwasResultLineEdit->text();
//            if (gwasResulFile.isEmpty())
//            {   // Gwas result file is necessary.
//                emit runningMsgWidgetAppendText(QDateTime::currentDateTime().toString() +
//                                                "\nA GWAS result file is necessary.\n");
//                QThread::msleep(10);
//                throw -1;
//            }

//            emit runningMsgWidgetAppendText(QDateTime::currentDateTime().toString() +
//                                            "\nMake qqman input file, \n");
//            QThread::msleep(10);
//            // Transform gwasResultFile to input file type of qqman.
//            QStringList qqmanFile = makeQQManInputFile(gwasResulFile); //   path/name.gemma_wald
//            QStringList outList;

//            if (qqmanFile.isEmpty())
//            {
//                emit runningMsgWidgetAppendText(QDateTime::currentDateTime().toString() +
//                                                "\nMake qqman input file ERROR. \n");
//                QThread::msleep(10);
//                throw -1;
//            }
//            emit runningMsgWidgetAppendText(QDateTime::currentDateTime().toString() +
//                                            "\nMake qqman input file OK. \n");
//            QThread::msleep(10);
//            for (auto item:qqmanFile)
//            {   // Multiple result, multiple output plot.
//                outList.append(this->workDirectory->getOutputDirectory()+"/"+this->workDirectory->getProjectName()
//                               +"_"+item.split(".")[item.split(".").length()-1]+"_qq.png");
//            }

//            emit runningMsgWidgetAppendText(QDateTime::currentDateTime().toString() +
//                                            "\nDraw QQ plot, \n");
//            QThread::msleep(10);
//            if (!this->drawQQplot(qqmanFile, outList))
//            {
//                emit runningMsgWidgetAppendText(QDateTime::currentDateTime().toString() +
//                                                "\nDraw QQ plot ERROR. \n");
//                QThread::msleep(10);
//                throw -1;
//            }

//            emit runningMsgWidgetAppendText(QDateTime::currentDateTime().toString() +
//                                            "\nDraw QQ plot OK. " +
//                                            "\nQQ plot: \n" + outList.join("\n") + "\n");
//            QThread::msleep(10);
//            QFile file;
//            for (auto item:qqmanFile)
//            {   // Remove intermediate file.
//                if (item == gwasResulFile)
//                {
//                    continue;
//                }
//                file.remove(item);
//            }
//        } catch (int) {
//            emit resetWindowSig();
//            QThread::msleep(10);
//        }
//    });
//    while (!fu.isFinished())
//    {
//        qApp->processEvents(QEventLoop::AllEvents, 200);
//    }

//    this->resetWindow();
//    this->runningFlag = false;
//}

/**
 * @brief MainWindow::drawManhattan
 *          Set parameter and call plot.R to draw manhattan plot.
 * @param data  input data file
 * @param out   output file
 * @return
 */
bool MainWindow::drawManhattan(QStringList data, QStringList out)
{
    if (data.isEmpty() || out.isEmpty() || data.size() != out.size())
    {
        return false;;
    }

    // Threshold value
    int gwBase =  ui->gwBaseLineEdit->text().toInt();
    int gwExpo = ui->gwExpoLineEdit->text().toInt();
    int sgBase = ui->sgBaseLineEdit->text().toInt();
    int sgExpo = ui->sgExpoLineEdit->text().toInt();

    QStringList param;

    for (int i = 0; i < data.size() && runningFlag; i++)
    {
        // The sequence of param is not changeable
        param.clear();
        param.append(this->scriptpath+"qqman/plot.R");
        param.append("manhattan");
#ifdef QT_NO_DEBUG
        param.append("debugno");    // Help to set path of manhattan.R and qq.R
#else
        param.append(this->scriptpath+"qqman/manhattan.R");
#endif
        param.append(data[i]);
        param.append(out[i]);
        param.append(QString::number(gwBase)+'e'+QString::number(gwExpo));
        param.append(QString::number(sgBase)+'e'+QString::number(sgExpo));

        // R in environment path is necessary.
        if (!runExTool("Rscript", param))
        {
            return false;
        }
    }
    // Show plot

//    if (this->runningFlag && checkoutExistence(out[0]))
//    {
//        emit setGraphViewerGraphSig(out);
//        QThread::msleep(10);
//    }

    return true;
}

/**
 * @brief MainWindow::drawQQplot
 *      Set parameter and call plot.R to draw QQ plot.
 * @param data  input data file
 * @param out   out file
 * @return
 */
bool MainWindow::drawQQplot(QStringList data, QStringList out)
{
    if (data.isEmpty() || out.isEmpty() || data.size() != out.size())
    {
        return false;;
    }

    QStringList param;

    for (int i = 0; i < data.size(); i++)
    {
        // The sequence of param is not changeable
        param.clear();
        param.append(this->scriptpath+"qqman/plot.R");
        param.append("qqplot");
#ifdef QT_NO_DEBUG
        param.append("debugno");    // Help to set path of manhattan.R and qq.R
#else
        param.append(this->scriptpath + "qqman/qq.R");
#endif
        param.append(data[i]);
        param.append(out[i]);
        // R in environment path is necessary.
        if (!runExTool("Rscript", param))
        {
            return false;
        }

    }
    // Show plot
//    if (this->runningFlag && checkoutExistence(out[0]))
//    {
//        emit setGraphViewerGraphSig(out);
//        QThread::msleep(10);
//    }
    return true;
}

/**
 * @brief MainWindow::on_gwasReultBrowButton_clicked
 *      To select gwas result file.
 */
//void MainWindow::on_qqmanGwasReultBrowButton_clicked()
//{
//    QFileDialog *fileDialog = new QFileDialog(this, "Open GWAS result file", this->workDirectory->getOutputDirectory(),
//                                              "result(*.linear *.logistic *.ps *.txt);;all(*)");
//    fileDialog->setViewMode(QFileDialog::Detail);

//    QStringList fileNames;
//    if (fileDialog->exec())
//    {
//        fileNames = fileDialog->selectedFiles();
//    }
//    delete fileDialog;
//    if (fileNames.isEmpty())    // If didn't choose any file.
//    {
//        return;
//    }
//    ui->qqmanGwasResultLineEdit->setText(fileNames[0]);
//}

/**
 * @brief MainWindow::makeQQmanFile
 * @param pvalueFile(generated by association tool)
 * @return  A file will be a input of manhattan.(header: SNP CHR BP P)
 */
QStringList MainWindow::makeQQManInputFile(QString pvalueFile)
{
    QStringList qqmanInFileList;
    if (pvalueFile.isNull() || pvalueFile.isEmpty())
    {
        return qqmanInFileList;
    }

    QFileInfo pvalueFileInfo(pvalueFile);
    QString pvalueFileAbPath = pvalueFileInfo.absolutePath();
    QString pvalueFileBaseName = pvalueFileInfo.baseName();
    QString pvalueFileSuffix = pvalueFileInfo.suffix();
    QString pvalueFileComSuffix = pvalueFileInfo.completeSuffix();

    if ( pvalueFileComSuffix == "assoc.linear" || pvalueFileComSuffix == "assoc.logistic")
    {   // Plink output file don't need to be transformed.
        qqmanInFileList.append(pvalueFile);
        return qqmanInFileList;
    }

    QFile gwasResultFile(pvalueFile);
    QTextStream gwasResultFileStream(&gwasResultFile);
    if (!gwasResultFile.open(QIODevice::ReadOnly))
    {
        emit setMsgBoxSig("Error", "Open gwasResultFile error.  ");
        return qqmanInFileList;
    }

    /* Script(one line):
     *  sed 's/:/\t/g; s/chr//g' 222_filter1_phe1_BN.ps
     *  |perl -alne '{if(/^0/){print "19\tchr0:$F[1]\t$F[0]\t$F[1]\t$F[3]"}
     *  else{print "$F[0]\tchr$F[0]:$F[1]\t$F[0]\t$F[1]\t$F[3]"}}'
     *  |sort -k 1,1n -k4,4n |perl -alne '{$num=shift @F;
     *  $line=join "\t",@F; print $line}'
     *  |sed '1 i\SNP\tCHR\tBP\tP' > phe1_emmax_table
     */
    if (pvalueFileSuffix == "ps")
    {   // Emmax output file.
        /* From: .ps:
         *      [SNP ID(CHR:BP)], [beta], [p-value] (header, don't exist in file)
         *      chr0:39616  0.7571908167    0.2146455451
         * To:
         *      SNP CHR BP  P (Header is necessary)
         */
        QFile qqmanInputFile(pvalueFileAbPath+"/"+pvalueFileBaseName+"."+"emmax");
        QTextStream qqmanInputFileStream(&qqmanInputFile);
        qqmanInputFile.open(QIODevice::ReadWrite);
        qqmanInputFileStream << "SNP\tCHR\tBP\tP" << endl; // Write header
        while (!gwasResultFileStream.atEnd())
        {   // Read data line by line.
            QStringList curLine = gwasResultFileStream.readLine().split(QRegExp("\\s+"), QString::SkipEmptyParts);
            QString SNP = curLine[0];

            if (SNP.split(":").length() < 2)
            {
                emit setMsgBoxSig("Error", ".ps file format error(maybe without chr).   ");
                return qqmanInFileList;
            }
            QString CHR = SNP.split(":")[0].remove(0, 3); // e.g. remove "chr" in "chr12", to get "12"
            QString BP = SNP.split(":")[1];
            QString P = curLine[curLine.length()-1];
            qqmanInputFileStream << SNP << "\t" << CHR << "\t" << BP << "\t" << P << endl;
        }
        qqmanInFileList.append(qqmanInputFile.fileName());
        qqmanInputFile.close();
        gwasResultFile.close();
        return qqmanInFileList;
    }

    /* Script (one line):
     *  perl -alne '{print "$F[1]\t$F[-1]"}'
     *  phe2_pc2_lmm.assoc.txt |sed 's/:/\t/g;
     *  s/chr//g' |perl -alne '{if(/^0/){print
     *  "chr0:$F[1]\t19\t$F[1]\t$F[2]"}
     *  else{print "chr$F[0]:$F[1]\t$F[0]\t$F[1]\t$F[2]"}}'
     *  |sort -k 2,2n -k3,3n |sed '1d' |sed '1 i\SNP\tCHR\tBP\tP'
     *  > phe2_pc2_gemma_lmm
     */
    if (pvalueFileComSuffix == "assoc.txt")
    {   // Gemma LMM
        /* From:
         *  chr	rs	ps	n_miss	allele1	allele0	af	beta	se	logl_H1	l_remle	l_mle	p_wald	p_lrt	p_score
         * To:
         *  SNP CHR BP  P (Header is necessary)
         */
        QStringList header = gwasResultFileStream.readLine().split(QRegExp("\\s+"), QString::SkipEmptyParts);    // Read header, we don't need it in .qqman file.
        int num = (header.length() != 12)  ? (header.length()-13+1) : 1;  // Multi(3) p-values in file when chosed all tested.
        for (int i = 1; i <= num; i++)
        {
            gwasResultFileStream.seek(0);       // Back to begin of the file.
            gwasResultFileStream.readLine();    // Read the header in the first line and strip it.
            QFile qqmanInputFile(pvalueFileAbPath+"/"+pvalueFileBaseName+"."+"gemma"
                                 +"_"+header[header.length()-i].split("_")[1]);    // out_name.gemma_wald
            QTextStream qqmanInputFileStream(&qqmanInputFile);
            qqmanInputFile.open(QIODevice::ReadWrite);
            qqmanInputFileStream << "SNP\tCHR\tBP\tP" << endl; // Write header
            while (!gwasResultFileStream.atEnd())
            {
                QStringList curLine = gwasResultFileStream.readLine().split(QRegExp("\\s+"), QString::SkipEmptyParts);
                QString SNP = curLine[1];
                QString CHR = curLine[0];
                QString BP = curLine[2];
                QString P = curLine[curLine.length()-i];
                qqmanInputFileStream << SNP << "\t" << CHR << "\t" << BP << "\t" << P << endl;
            }
            qqmanInFileList.append(qqmanInputFile.fileName());
            qqmanInputFile.close();
        }
        gwasResultFile.close();
        return qqmanInFileList;
    }

    return qqmanInFileList;
}

void MainWindow::on_pcaRunPushButton_clicked()
{

}

void MainWindow::on_ldRunPushButton_clicked()
{

}

void MainWindow::runPopLDdecaybyFamily(void)
{

}

void MainWindow::runPopLDdecaySingle(void)
{

}

bool MainWindow::ldPlot(QStringList ldResultFileList)
{


    return true;
}

void MainWindow::graphViewer_clicked_slot()
{
    qDebug() << "Graph viewer clicked" << endl;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    qDebug() << "MainWindow::closeEvent";
    this->resetWindow();
    if (this->runningMsgWidget->isVisible())
    {
        this->runningMsgWidget->close();
    }

    if (this->gemmaParamWidget->isVisible())
    {
        this->gemmaParamWidget->close();
    }
    if (this->emmaxParamWidget->isVisible())
    {
        this->emmaxParamWidget->close();
    }
    if (this->qualityControl->isVisible())
    {
        this->qualityControl->close();
    }
    if (this->isVisible() && !runningMsgWidget->isVisible())
    {
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

void MainWindow::on_refGeneFileBrowButton_clicked()
{
    QFileDialog *fileDialog = new QFileDialog(this, "Open reference gene file", "",
                                              "gff(*.gff *.gff2 *.gff3 *gff.txt);;gtf(*.gtf *.gtf3 *.gtf.txt);;refGene(*refGene.txt);;all(*)");
    fileDialog->setViewMode(QFileDialog::Detail);

    QStringList fileNames;
    if (fileDialog->exec())
    {
        fileNames = fileDialog->selectedFiles();
    }
    delete fileDialog;
    if (fileNames.isEmpty())    // If didn't choose any file.
    {
        return;
    }
    ui->refGeneFileLineEdit->setText(fileNames[0]);
}

void MainWindow::on_refSeqFileBrowButton_clicked()
{
    QFileDialog *fileDialog = new QFileDialog(this, "Open reference sequence file", "",
                                              "fasta(*.fa *.Fa *.fasta *.Fasta);;all(*)");
    fileDialog->setViewMode(QFileDialog::Detail);

    QStringList fileNames;
    if (fileDialog->exec())
    {
        fileNames = fileDialog->selectedFiles();
    }
    delete fileDialog;
    if (fileNames.isEmpty())    // If didn't choose any file.
    {
        return;
    }
    ui->refSeqFileLineEdit->setText(fileNames[0]);
}

void MainWindow::on_avinFileBrowButton_clicked()
{
    QFileDialog *fileDialog = new QFileDialog(this, "Open annovar input file file", "",
                                              "avinput(*.avinput *.txt *.);;all(*)");
    fileDialog->setViewMode(QFileDialog::Detail);

    QStringList fileNames;
    if (fileDialog->exec())
    {
        fileNames = fileDialog->selectedFiles();
    }
    delete fileDialog;
    if (fileNames.isEmpty())    // If didn't choose any file.
    {
        return;
    }
    ui->avinFileLineEdit->setText(fileNames[0]);
}

/**
 * @brief MainWindow::on_snpPosBrowButton_clicked
 *      To open postion of SNP file.
 */
void MainWindow::on_snpPosFileFileBrowButton_clicked()
{
    QFileDialog *fileDialog = new QFileDialog(this, "Open SNP position file", "",
                                              "snpPos(all(*)");
    fileDialog->setViewMode(QFileDialog::Detail);

    QStringList fileNames;
    if (fileDialog->exec())
    {
        fileNames = fileDialog->selectedFiles();
    }
    delete fileDialog;
    if (fileNames.isEmpty())    // If didn't choose any file.
    {
        return;
    }
    ui->snpPosFileLineEdit->setText(fileNames[0]);
}

/**
 * @brief MainWindow::on_baseFileBrowButton_clicked
 *          To open functional annotaion reference file.
 */
void MainWindow::on_funcAnnoRefFileBrowButton_clicked()
{
    QFileDialog *fileDialog = new QFileDialog(this, "Open functional annotation reference file", "",
                                              "funcAnnoRef(funcanno(*.funcanno *ncbi.csv *ensem.csv);;all(*)");
    fileDialog->setViewMode(QFileDialog::Detail);

    QStringList fileNames;
    if (fileDialog->exec())
    {
        fileNames = fileDialog->selectedFiles();
    }
    delete fileDialog;
    if (fileNames.isEmpty())    // If didn't choose any file.
    {
        return;
    }
    ui->funcAnnoRefFileLineEdit->setText(fileNames[0]);
}

void MainWindow::on_annoPvalBrowButton_clicked()
{
    QFileDialog *fileDialog = new QFileDialog(this, "Open GWAS result file", this->workDirectory->getOutputDirectory(),
                                              "result(*.linear *.logistic *.ps *.txt);;all(*)");
    fileDialog->setViewMode(QFileDialog::Detail);

    QStringList fileNames;
    if (fileDialog->exec())
    {
        fileNames = fileDialog->selectedFiles();
    }
    delete fileDialog;
    if (fileNames.isEmpty())    // If didn't choose any file.
    {
        return;
    }
    ui->annoPvalLineEdit->setText(fileNames[0]);
}

/**
 * @brief AssocTool::pValCorrect
 * @param pvalFile  The last column is p-val.
 * @param type      Shep-Down(Holm):holm, FDR(BH):BH, Bonferroni:bonferroni
 * @return
 */
bool MainWindow::pValCorrect(QString pvalFile, bool header, QString correctType, QString outFile)
{
    if (pvalFile.isNull() || correctType.isNull() || outFile.isNull())
    {
        return false;
    }

    QStringList param;
    // The sequence of param is not changeable
    param.clear();
    param.append(this->scriptpath+"qqman/correction.R");
    param.append(pvalFile);
    param.append(header ? "TRUE" : "FALSE");
    param.append(correctType);
    param.append(outFile);
    // R in environment path is necessary.
    if (!runExTool("Rscript", param))
    {
        return false;
    }

    return true;
}

void MainWindow::on_qualCtrlDetailPushButton_clicked()
{
    this->qualityControl->show();
}

bool MainWindow::runExTool(QString tool, QStringList param)
{


    return 1;
}

bool MainWindow::checkoutExistence(QString filePath)
{
    if (filePath.isNull() || filePath.isEmpty())
    {
        return false;
    }
    QFile file(filePath);
    return file.exists();
}

void MainWindow::on_outMessageReady(const QString text)
{
    if (this->runningFlag)
    {
        this->runningMsgWidget->setText(this->refreshMessage(this->runningMsgWidget->getText(), text));
        this->runningMsgWidget->repaint();
        qApp->processEvents();
    }
}

void MainWindow::on_errMessageReady(const QString text)
{
    if (this->runningFlag)
    {
        this->runningMsgWidget->appendText(text);
        this->runningMsgWidget->repaint();
        qApp->processEvents();
    }
}

// Mutilple thread slot function.
void MainWindow::on_setLineEditText(QLineEdit *lineEdit, QString text)
{
    if (this->runningFlag)
    {
        lineEdit->setText(text);
    }
}

// Mutilple thread slot function.
void MainWindow::on_setButtonEnabled(QAbstractButton *button, bool boolean)
{
    button->setEnabled(boolean);
}

// Mutilple thread slot function.
void MainWindow::on_setGraphViewerGraph(QStringList plot)
{

}

void MainWindow::on_resetWindowSig()
{
    this->resetWindow();
}

void MainWindow::on_setMsgBoxSig(const QString &title, const QString &text)
{
    QMessageBox::information(nullptr, title, text);
}

/**
 * @brief MainWindow::addFilesExecutePermission
 *          Add execute permission of files in directory.(All file in directory)
 * @param directory     A path QString, which contains files need to add execute permission.
 */
void MainWindow::addFilesExecutePermission(QString directory)
{
    QDir dir(directory);
    QStringList fileList = dir.entryList();
    for (auto item:fileList)
    {
        QProcess::execute("chmod", QStringList()<< "+x" << directory+item);
    }
}

void MainWindow::structuralAnnotation(QString avinputFilePath, QString refGeneFilePath,
                                      QString refSeqFilePath, QString outFile)
{

}

void MainWindow::functionalAnnotation(QString snpPosFilePath, QString varFuncFilePath,
                                      QString exVarFuncFilePath, QString funcAnnoBaseFilePath)
{

}

void MainWindow::on_annotationRunButton_clicked()
{

}

/**
 * @brief MainWindow::on_annoStepPushButton_clicked
 *              Make avinputFile for structural annotation and
 *            snpPosFile for functional annotaion from vcfFile
 *            and association p-value file.
 */
void MainWindow::on_annoStepPushButton_clicked()
{

}

void MainWindow::on_compleFIDRadioButton_clicked()
{
    ui->fidFileLineEdit->setEnabled(ui->compleFIDRadioButton->isChecked());
    ui->fidFileBrowButton->setEnabled(ui->compleFIDRadioButton->isChecked());
//    ui->fidWarnLabel->setHidden(!ui->compleFIDRadioButton->isChecked());
}

void MainWindow::on_filterChrFileBrowButton_clicked()
{
    QFileDialog *fileDialog = new QFileDialog(this, "Open chr list file", "", "all(*)");
    fileDialog->setViewMode(QFileDialog::Detail);

    QStringList fileNames;
    if (fileDialog->exec())
    {
        fileNames = fileDialog->selectedFiles();
    }
    delete fileDialog;
    if (fileNames.isEmpty())    // If didn't choose any file.
    {
        return;
    }

    ui->filterChrFileLineEdit->setText(fileNames[0]);
}

void MainWindow::on_filterChrRadioButton_clicked()
{
    ui->filterChrFileLineEdit->setEnabled(ui->filterChrRadioButton->isChecked());
    ui->filterChrFileBrowButton->setEnabled(ui->filterChrRadioButton->isChecked());
}

void MainWindow::on_fidFileBrowButton_clicked()
{
    QFileDialog *fileDialog = new QFileDialog(this, "Open FID info file", "", "all(*)");
    fileDialog->setViewMode(QFileDialog::Detail);

    QStringList fileNames;
    if (fileDialog->exec())
    {
        fileNames = fileDialog->selectedFiles();
    }
    delete fileDialog;
    if (fileNames.isEmpty())    // If didn't choose any file.
    {
        return;
    }

    ui->fidFileLineEdit->setText(fileNames[0]);
}

bool MainWindow::completeSnpID(QString genotype)
{
    if (genotype.isNull())
    {
        return false;
    }

    QFileInfo fileInfo(genotype);
    QString genoFileAbPath = fileInfo.absolutePath();
    QString genoFileBaseName = fileInfo.baseName();
    QString genoFileSuffix = fileInfo.suffix();
    emit runningMsgWidgetAppendText("Complete SNP ID, \n");
    bool completeFlag = true;
    try {
        if (isVcfFile(genotype))
        {
            if (!fileReader->completeSnpID(genotype))
            {
                throw -1;
            }
        }
        if (genoFileSuffix == "ped")
        {
            if (!fileReader->completeSnpID(genoFileAbPath+"/"+genoFileBaseName+".map"))
            {
                throw -1;
            }
        }
        else if (genoFileSuffix == "tped")
        {
            if (!fileReader->completeSnpID(genotype))
            {
                throw -1;
            }
        }
        else if (genoFileSuffix == "bed")
        {
            if (!fileReader->completeSnpID(genoFileAbPath+"/"+genoFileBaseName+".bim"))
            {
                throw -1;
            }
        }
    } catch (...) {
        completeFlag = false;
        emit runningMsgWidgetAppendText("Complete SNP ID ERROR.\n");
    }
    if (completeFlag)
    {
        emit runningMsgWidgetAppendText("Complete SNP ID OK.\n");
    }

    return true;
}
