// bank_system_b2.cpp
// Compile: g++ -std=c++17 bank_system_b2.cpp -o bank_system_b2

#include <iostream>
#include <vector>
#include <string>
#include <iomanip>

// -------------------- Transaction --------------------
class Transaction {
public:
    enum Type { DEPOSIT, WITHDRAWAL, TRANSFER_IN, TRANSFER_OUT, INTEREST };

    Transaction(double amount = 0.0, Type type = DEPOSIT, const std::string& date = "N/A",
        const std::string& note = "", double balanceAfter = 0.0)
        : amount(amount), type(type), date(date), note(note), balanceAfter(balanceAfter) {
    }

    double getAmount() const { return amount; }
    Type getType() const { return type; }
    std::string getDate() const { return date; }
    std::string getNote() const { return note; }
    double getBalanceAfter() const { return balanceAfter; }

    std::string typeToString() const {
        switch (type) {
        case DEPOSIT: return "Deposit";
        case WITHDRAWAL: return "Withdrawal";
        case TRANSFER_IN: return "Transfer In";
        case TRANSFER_OUT: return "Transfer Out";
        case INTEREST: return "Interest";
        default: return "Unknown";
        }
    }

private:
    double amount;
    Type type;
    std::string date;
    std::string note;
    double balanceAfter;
};

// -------------------- Account (base) --------------------
class Account {
public:
    Account(const std::string& accNo, const std::string& owner, double initBalance = 0.0)
        : accountNumber(accNo), ownerName(owner), balance(initBalance), startingBalance(initBalance) {
    }

    virtual ~Account() {}

    std::string getAccountNumber() const { return accountNumber; }
    std::string getOwnerName() const { return ownerName; }
    double getBalance() const { return balance; }
    double getStartingBalance() const { return startingBalance; }

    virtual void deposit(double amount, const std::string& date = "N/A", const std::string& note = "") {
        if (amount <= 0.0) {
            std::cout << "Deposit amount must be positive.\n";
            return;
        }
        balance += amount;
        Transaction t(amount, Transaction::DEPOSIT, date, note, balance);
        (*this) += t;
    }

    virtual bool withdraw(double amount, const std::string& date = "N/A", const std::string& note = "") {
        if (amount <= 0.0) return false;
        if (amount > balance) return false;
        balance -= amount;
        Transaction t(amount, Transaction::WITHDRAWAL, date, note, balance);
        (*this) += t;
        return true;
    }

    bool transferTo(Account& target, double amount, const std::string& date = "N/A", const std::string& note = "") {
        if (amount <= 0.0 || amount > balance) return false;
        balance -= amount;
        Transaction tout(amount, Transaction::TRANSFER_OUT, date, note + (note.empty() ? "" : " ") + "(to " + target.getAccountNumber() + ")", balance);
        (*this) += tout;

        target.balance += amount;
        Transaction tin(amount, Transaction::TRANSFER_IN, date, note + (note.empty() ? "" : " ") + "(from " + this->getAccountNumber() + ")", target.balance);
        target += tin;
        return true;
    }

    Account& operator+=(const Transaction& t) {
        history.push_back(t);
        return *this;
    }

    bool operator==(const Account& other) const {
        return this->accountNumber == other.accountNumber;
    }

    virtual void applyInterest(const std::string& date = "N/A") {}

    void printStatement(const std::string& customerShortId) const {
        std::cout << ownerName << " (ID: " << customerShortId
            << ", Account: " << accountNumber
            << ", Starting Balance: " << std::fixed << std::setprecision(2)
            << startingBalance << ")\n";
        for (const auto& t : history) {
            std::cout << "  [" << t.getDate() << "] "
                << t.typeToString() << " "
                << std::fixed << std::setprecision(2) << t.getAmount()
                << " (" << t.getNote() << ")"
                << " → Balance: " << std::fixed << std::setprecision(2) << t.getBalanceAfter()
                << "\n";
        }
        std::cout << "Final Balance: " << std::fixed << std::setprecision(2) << balance << "\n";
        std::cout << "-----------------------------------\n\n";
    }

protected:
    std::string accountNumber;
    std::string ownerName;
    double balance;
    double startingBalance;
    std::vector<Transaction> history;
};

// -------------------- SavingsAccount (derived) --------------------
class SavingsAccount : public Account {
public:
    SavingsAccount(const std::string& accNo, const std::string& owner, double initBalance = 0.0,
        double interestRatePercent = 0.0, int limitPerMonth = 3, double fee = 2.0)
        : Account(accNo, owner, initBalance),
        interestRatePercent(interestRatePercent),
        withdrawLimitPerMonth(limitPerMonth),
        withdrawCountThisMonth(0),
        withdrawalFee(fee) {
    }

    bool withdraw(double amount, const std::string& date = "N/A", const std::string& note = "") override {
        if (amount <= 0.0) return false;
        double totalDeduct = amount;
        bool feeApplied = false;
        if (withdrawCountThisMonth >= withdrawLimitPerMonth) {
            totalDeduct += withdrawalFee;
            feeApplied = true;
        }
        if (totalDeduct > balance) return false;
        balance -= totalDeduct;
        std::string noteMain = note + (feeApplied ? " (fee applied)" : "");
        history.emplace_back(amount, Transaction::WITHDRAWAL, date, noteMain, balance);
        if (feeApplied) {
            history.emplace_back(withdrawalFee, Transaction::WITHDRAWAL, date, "Withdrawal fee", balance);
        }
        withdrawCountThisMonth++;
        return true;
    }

    void applyInterest(const std::string& date = "N/A") override {
        double interest = balance * (interestRatePercent / 100.0);
        balance += interest;
        history.emplace_back(interest, Transaction::INTEREST, date, "Interest Applied", balance);
    }

    void resetWithdrawCount() { withdrawCountThisMonth = 0; }

private:
    double interestRatePercent;
    int withdrawLimitPerMonth;
    int withdrawCountThisMonth;
    double withdrawalFee;
};

// -------------------- Customer --------------------
class Customer {
public:
    Customer(const std::string& shortId, const std::string& name) : id(shortId), name(name) {}

    void addAccount(Account* acc) { accounts.push_back(acc); }

    void printPortfolio() const {
        std::cout << "Customer " << name << " (ID: " << id << ")\n";
        for (const auto acc : accounts) {
            acc->printStatement(id);
        }
    }

private:
    std::string id;
    std::string name;
    std::vector<Account*> accounts;
};

// -------------------- Main / Tests --------------------
int main() {
    // Accounts and customers renamed: Phuc, Loc, Tho
    Account phucAcc("10001", "Phuc", 800.0);
    SavingsAccount locAcc("20001", "Loc", 1200.0, 3.0, 2, 5.0);
    SavingsAccount thoAcc("30001", "Tho", 2000.0, 3.0, 3, 2.0);

    Customer phuc("01", "Phuc");
    Customer loc("02", "Loc");
    Customer tho("03", "Tho");

    phuc.addAccount(&phucAcc);
    loc.addAccount(&locAcc);
    tho.addAccount(&thoAcc);

    // Transactions
    phucAcc.deposit(200, "2025-09-17", "Paycheck");
    phucAcc.withdraw(100, "2025-09-17", "ATM");
    phucAcc.transferTo(locAcc, 150, "2025-09-17", "Pay Loc");

    locAcc.deposit(300, "2025-09-19", "Bonus");
    locAcc.withdraw(50, "2025-09-20", "Groceries");
    locAcc.withdraw(25, "2025-09-21", "Extra1");
    locAcc.withdraw(30, "2025-09-22", "Extra2");
    locAcc.applyInterest("2025-09-30");

    thoAcc.deposit(500, "2025-09-17", "Bonus");
    thoAcc.withdraw(250, "2025-09-17", "Shopping");
    thoAcc.applyInterest("2025-09-30");

    // ===== Thêm chuyển tiền giữa user vào luôn =====
    phucAcc.transferTo(locAcc, 300.00, "2025-10-01", "Phuc sends money to Loc");
    locAcc.transferTo(thoAcc, 100.00, "2025-10-01", "Loc sends money to Tho");

    // Print portfolios (chỉ cần in 1 lần)
    phuc.printPortfolio();
    loc.printPortfolio();
    tho.printPortfolio();

    return 0;
}

